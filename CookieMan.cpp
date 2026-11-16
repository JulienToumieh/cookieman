
#include <iostream>
#include <filesystem>
#include <fstream>
#include <regex>
#include <string>
#include <set>
#include <map>
#include <sstream>
#include <vector>
#include <algorithm>

namespace fs = std::filesystem;

int main(int argc, char* argv[])
{
    fs::path directory = (argc > 1) ? argv[1] : ".";

    directory = fs::absolute(directory).lexically_normal();

    if (!fs::exists(directory) || !fs::is_directory(directory))
    {
        std::cerr << "Invalid directory: " << directory << '\n';
        return 1;
    }

    std::regex pattern(R"(\{\{\s*(\S(?:.*?\S)?)\s*\}\})");

    std::set<std::string> matches;
    std::map<std::string, std::string> replacements;

    auto search = [&](const std::string& content)
    {
        std::sregex_iterator it(content.begin(), content.end(), pattern);
        std::sregex_iterator end;

        for (; it != end; ++it)
        {
            std::string match = (*it)[1].str();

            if (!match.empty())
                matches.insert(match);
        }
    };

    std::vector<fs::path> entries;
    search(directory.filename().string());

    for (const auto& entry : fs::recursive_directory_iterator(
             directory,
             fs::directory_options::skip_permission_denied))
    {
        entries.push_back(entry.path());

        search(entry.path().filename().string());

        if (entry.is_regular_file())
        {
            std::ifstream file(entry.path(), std::ios::binary);

            if (file.is_open())
            {
                std::ostringstream buffer;
                buffer << file.rdbuf();
                search(buffer.str());
            }
        }
    }

    std::cout << "Found " << matches.size()
              << " unique placeholders:\n\n";

    for (const auto& match : matches)
    {
        std::cout << "{{ " << match << " }}: ";

        std::string replacement;
        std::getline(std::cin, replacement);

        replacements[match] = replacement;
    }

    auto replaceAll = [&](std::string content)
    {
        std::string result;

        std::sregex_iterator it(content.begin(), content.end(), pattern);
        std::sregex_iterator end;

        size_t lastPos = 0;

        for (; it != end; ++it)
        {
            size_t pos = it->position();
            size_t len = it->length();

            result.append(content, lastPos, pos - lastPos);

            std::string key = (*it)[1].str();

            auto replacement = replacements.find(key);

            if (replacement != replacements.end())
                result += replacement->second;
            else
                result += it->str();

            lastPos = pos + len;
        }

        result.append(content, lastPos, std::string::npos);

        return result;
    };

    std::cout << "\nApplying replacements...\n";

    for (const auto& entry : entries)
    {
        if (!fs::exists(entry) || !fs::is_regular_file(entry))
            continue;

        std::ifstream file(entry, std::ios::binary);

        if (!file.is_open())
            continue;

        std::ostringstream buffer;
        buffer << file.rdbuf();
        file.close();

        std::string content = buffer.str();
        std::string updated = replaceAll(content);

        if (updated != content)
        {
            std::ofstream output(
                entry,
                std::ios::binary | std::ios::trunc
            );

            if (output)
            {
                output << updated;

                std::cout << "Modified: " << fs::relative(entry, directory).string() << '\n';
            }
        }
    }

    std::sort(entries.begin(), entries.end(),
        [](const fs::path& a, const fs::path& b)
        {
            return std::distance(a.begin(), a.end()) >
                   std::distance(b.begin(), b.end());
        });

    for (const auto& entry : entries)
    {
        if (!fs::exists(entry))
            continue;

        std::string oldName = entry.filename().string();
        std::string newName = replaceAll(oldName);

        if (oldName != newName)
        {
            fs::path newPath = entry.parent_path() / newName;

            try
            {
                if (fs::exists(newPath))
                {
                    std::cerr << "Skipping rename (destination exists): "
                              << newPath << '\n';
                    continue;
                }

                fs::rename(entry, newPath);

                std::cout << "Renamed: "
                    << entry.lexically_relative(directory).string()
                    << " -> "
                    << newPath.lexically_relative(directory).string()
                    << '\n';
            }
            catch (const fs::filesystem_error& e)
            {
                std::cerr << "Rename failed: " << e.what() << '\n';
            }
        }
    }

    std::string oldRootName = directory.filename().string();
    std::string newRootName = replaceAll(oldRootName);

    if (oldRootName != newRootName)
    {
        fs::path newDirectory = directory.parent_path() / newRootName;

        try
        {
            if (fs::exists(newDirectory))
            {
                std::cerr << "Skipping root rename (destination exists): "
                        << newDirectory << '\n';
            }
            else
            {
                fs::rename(directory, newDirectory);

                std::cout << "Renamed root folder: "
                        << oldRootName << " -> " << newRootName << '\n';
            }
        }
        catch (const fs::filesystem_error& e)
        {
            std::cerr << "Root folder rename failed: "
                    << e.what() << '\n';
        }
    }

    std::cout << "\nDone!\n";

    return 0;
}
