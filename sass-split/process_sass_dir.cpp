#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <set>
#include <sstream>    // For std::istringstream
#include <iterator>   // For std::istream_iterator
#include <cstring>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>

// Split a string into tokens by whitespace
std::vector<std::string> split(const std::string& str) {
    std::istringstream iss(str);
    std::vector<std::string> tokens{std::istream_iterator<std::string>{iss},
                                    std::istream_iterator<std::string>{}};
    return tokens;
}

std::string getFullPath(const std::string& path, const std::string& file) {
    return path + "/" + file;
}

bool fileExists(const std::string& path) {
    struct stat buffer;   
    return (stat(path.c_str(), &buffer) == 0); 
}

bool isSassFile(const std::string& file) {
    const std::string sass_ext = ".sass";
    const std::string split_sass_ext = ".split.sass";
    // Check if the file ends with ".sass"
    if (file.size() >= sass_ext.size() && 
        file.substr(file.size() - sass_ext.size()) == sass_ext) 
    {
        // Further check to ensure it does not end with ".split.sass"
        if (file.size() >= split_sass_ext.size() && 
            file.substr(file.size() - split_sass_ext.size()) == split_sass_ext) 
        {
            return false;  // It's a ".split.sass" file, not what we want.
        }
        return true;  // It's a ".sass" file and not a ".split.sass" file.
    }
    return false;  // It does not end with ".sass"
}


int main(int argc, char *argv[]) {
    if (argc != 3 || std::string(argv[1]) != "--dir") {
        std::cerr << "Usage: " << argv[0] << " --dir <directory_of_sass_files>\n";
        return 1;
    }

    std::string sass_dir = argv[2];
    DIR *dir;
    struct dirent *ent;
    std::vector<std::string> sass_files;

    if ((dir = opendir(sass_dir.c_str())) != NULL) {
        while ((ent = readdir(dir)) != NULL) {
            if(isSassFile(ent->d_name)) {
                std::cout << "Found sass file: " << ent->d_name << std::endl;
                sass_files.push_back(getFullPath(sass_dir, std::string(ent->d_name)));
            } else {
                if(strcmp(ent->d_name, ".") != 0 && strcmp(ent->d_name, "..") != 0) {
                    std::string filePath = getFullPath(sass_dir, std::string(ent->d_name));
                    // Delete non-sass files
                    if(std::remove(filePath.c_str()) == 0) {
                        std::cout << "Deleted non-sass file: " << ent->d_name << std::endl;
                    } else {
                        std::cerr << "Error deleting file: " << ent->d_name << std::endl;
                    }
                }
            }
        }
        closedir(dir);
    } else {
        perror("");
        return EXIT_FAILURE;
    }

    

    for (const auto& sass_file : sass_files) {
        std::map<std::pair<int, int>, std::ofstream> f_open;
        std::set<int> have_created_gwarp_ids;

        std::cout << "Processing " << sass_file << "\n";
        std::ifstream file(sass_file);
        std::string content((std::istreambuf_iterator<char>(file)),
                             std::istreambuf_iterator<char>());
                             
        auto tokens = split(content);
        auto underscorePos = sass_file.find_last_of('_');
        int kernel_id = std::stoi(sass_file.substr(underscorePos + 1, sass_file.find(".sass") - underscorePos - 1));

        for (size_t i = 0; i < tokens.size() / 3; ++i) {
            int gwarp_id = std::stoi(tokens[i*3 + 2], nullptr, 16);
            if (have_created_gwarp_ids.find(gwarp_id) == have_created_gwarp_ids.end()) {
                have_created_gwarp_ids.insert(gwarp_id);
                std::string outputPath = sass_dir + 
                                         "/kernel_" + 
                                         std::to_string(kernel_id) + 
                                         "_gwarp_id_" + 
                                         std::to_string(gwarp_id) + 
                                         ".split.sass";
                if (!fileExists(outputPath)) {
                    f_open[{kernel_id, gwarp_id}].open(outputPath);
                }
            }
            f_open[{kernel_id, gwarp_id}] << tokens[i*3] << " " << tokens[i*3 + 1] << "\n";
        }

        for (auto& item : f_open) {
            item.second.close();
        }
    }

    return 0;
}
