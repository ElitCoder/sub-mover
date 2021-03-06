#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <map>
#include <regex>
#include <vector>

using namespace std;

struct Episode {
    string path;
    int season;
    int episode;
};

using FileList = vector<string>;
using EpisodeList = vector<Episode>;
using EpisodeMap = map<string, Episode>;

// Regex match for e.g. "S01E01"
static const string REGEX_EPISODE = "[sS](\\d{1,2})[eE](\\d{1,2})";
// Regex match for e.g. "0101 - Episode (...)"
static const string REGEX_EPISODE_FALLBACK = "(\\d{2})(\\d{2})";
// Regex match for e.g. "1x01 - Episode (...)"
static const string REGEX_EPISODE_FALLBACK_2 = "(\\d{1,2})x(\\d{1,2})";
// Regex match for e.g. "01 - Episode (...)"
static const string REGEX_EPISODE_FALLBACK_3 = "(\\d{2}) -";
// Regex match for e.g. ".101-"
static const string REGEX_EPISODE_FALLBACK_4 = "\\.(\\d{1})(\\d{2})\\-";
// Video formats
static const string VIDEO_FORMATS = ".mkv|.mp4|.avi";
// Subtitle formats
static const string SUBTITLE_FORMATS = ".srt";

void print_help(const string &program)
{
    cout << "Usage: " << program << " <subdir> <videodir> [overwrite]\n";
}

bool string_match(const string &str, const string &match, vector<string> &matches)
{
    regex reg(match);
    smatch m;

    if (!regex_search(str, m, reg)) {
        return false;
    }

    for (auto &v : m) {
        matches.push_back(v);
    }

    return true;
}

FileList get_files_in_folder(const string &path, const string &extension_regex)
{
    FileList filenames;

    try {
        for (auto &filename : filesystem::directory_iterator(path)) {
            vector<string> matches;
            if (string_match(filename.path(), extension_regex, matches)) {
                cout << "Found file: " << filename.path() << endl;
                filenames.push_back(filename.path());
            }
        }
    } catch (...) {
        // Can't open directory etc.
        cout << "Failed to open directory '" << path << "'!\n";
    }

    return filenames;
}

EpisodeList populate_episodes(FileList files)
{
    EpisodeList episodes;

    for (auto &file : files) {
        vector<string> matches;
        // Regex match for season and episode information
        if (!string_match(file, REGEX_EPISODE, matches) &&
            !string_match(file, REGEX_EPISODE_FALLBACK, matches) &&
            !string_match(file, REGEX_EPISODE_FALLBACK_2, matches) &&
            !string_match(file, REGEX_EPISODE_FALLBACK_3, matches) &&
            !string_match(file, REGEX_EPISODE_FALLBACK_4, matches)) {
            // Not an episode file
            continue;
        }
        // Remove first match since it's the full regex match
        matches.erase(matches.begin());
        // Either:
        //   [0] = season
        //   [1] = episode
        // or
        //   [0] = episode
        // depending on the regex match.
        Episode episode;
        episode.path = file;
        if (matches.size() == 1) {
            episode.season = -1; // Auto-pick from matching episode file
            // [0] = episode
            episode.episode = stoi(matches.at(0));
        } else {
            episode.season = stoi(matches.at(0));
            episode.episode = stoi(matches.at(1));
        }
        episodes.push_back(episode);
    }

    // Sort in season + episode order
    sort(episodes.begin(), episodes.end(), [] (auto &a, auto &b) {
        if (a.season == b.season) {
            return a.episode < b.episode;
        } else {
            return a.season < b.season;
        }
    });

    for (auto &episode : episodes) {
        cout << "Found season " << episode.season << " episode " << episode.episode << endl;
    }

    return episodes;
}

EpisodeMap map_episodes(EpisodeList vids, EpisodeList subs)
{
    EpisodeMap map;

    for (auto &video : vids) {
        for (auto &sub : subs) {
            if (video.episode == sub.episode) {
                if (video.season >= 0 && sub.season >= 0 &&
                    video.season != sub.season) {
                    continue;
                }
                map[video.path] = sub;
            }
        }
    }

    return map;
}

void copy_subtitles(EpisodeMap map, bool overwrite)
{
    for (auto const &[video_path, sub] : map) {
        cout << "Video file: " << video_path << endl;
        cout << "Subtitle file: " << sub.path << endl;

        string path = filesystem::path(video_path).parent_path();
        string filename = filesystem::path(video_path).stem();
        string extension = filesystem::path(sub.path).extension();
        auto new_sub_path = path + "/" + filename + extension;
        cout << "Copying subtitle " << sub.path << " to " << new_sub_path << endl;
        try {
            if (overwrite) {
                // Remove already existing file before copying
                filesystem::remove(new_sub_path);
            }
            filesystem::copy(sub.path, new_sub_path);
        } catch (...) {
            if (overwrite) {
                cout << "Failed to copy subtitle, permission denied?\n";
            } else {
                cout << "Failed to copy subtitle, file already exists?\n";
            }
        }
    }

    cout << "\nCopied " << map.size() << " subtitle(s)\n";
}

int main(int argc, char *argv[])
{
    if (argc < 3) {
        print_help(argv[0]);
        exit(EXIT_FAILURE);
    }

    bool overwrite = (argc >= 4) ? (string(argv[3]) == "overwrite") : false;

    cout << "Finding subtitle files...\n";
    auto sub_files = get_files_in_folder(argv[1], SUBTITLE_FORMATS);
    cout << "\nFinding video files...\n";
    auto video_files = get_files_in_folder(argv[2], VIDEO_FORMATS);

    cout << "\nResolving subtitle files...\n";
    auto subs = populate_episodes(sub_files);
    cout << "\nResolving video files...\n";
    auto vids = populate_episodes(video_files);

    EpisodeMap mapped;
    // Handle the case where there is only a video and a subtitle file and both
    // of them are missing episode information. It's probably a movie.
    if (subs.empty() && vids.empty() && (sub_files.size() == 1) && (video_files.size() == 1)) {
        cout << "\nOnly found one subtitle and one video file, mapping them together\n";
        Episode episode;
        episode.path = sub_files.front();
        mapped[video_files.front()] = episode;
    } else {
        // Normal episode mapping
        cout << "\nMapping videos <-> subtitles...\n";
        mapped = map_episodes(vids, subs);
    }

    cout << "\nCopying subtitles...\n";
    copy_subtitles(mapped, overwrite);
    return 0;
}
