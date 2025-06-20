#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <chrono>
#include <algorithm>
#include <vector>      
#include <map>         
#include <set>


using namespace std;
namespace fs = std::filesystem;

// Helper: Trim whitespace
string trim(const string& str) {
    auto start = find_if_not(str.begin(), str.end(), ::isspace);
    auto end = find_if_not(str.rbegin(), str.rend(), ::isspace).base();
    return (start < end) ? string(start, end) : "";
}

// Simple hash function
string simpleHash(const string& content) {
    hash<string> hasher;
    size_t hashed = hasher(content);
    return to_string(hashed);
}

// Initialize repository
void init() {
    if (!fs::exists(".minigit")) {
        fs::create_directory(".minigit");
        ofstream(".minigit/index").close();
        ofstream(".minigit/HEAD").close();
        cout << "Initialized empty MiniGit repository in .minigit/\n";
    } else {
        cout << "MiniGit repository already exists.\n";
    }
}

// Add file to staging
void addFile(const string& filename) {
    ifstream inFile(filename);
    if (!inFile) {
        cout << "ERROR: File '" << filename << "' not found.\n";
        return;
    }

    stringstream buffer;
    buffer << inFile.rdbuf();
    string content = buffer.str();
    inFile.close();

    string hash = simpleHash(content);

    ofstream indexFile(".minigit/index", ios::app);
    indexFile << filename << " " << hash << "\n";
    indexFile.close();

    cout << "Added '" << filename << "' to staging area.\n";
}

// Commit staged files
void commit(const string& message) {
    ifstream indexFile(".minigit/index");
    if (!indexFile) {
        cout << "ERROR: Staging area not found.\n";
        return;
    }

    stringstream commitContent;
    commitContent << "message: " << message << "\n";

    ifstream headFile(".minigit/HEAD");
    string parentHash;
    getline(headFile, parentHash);
    headFile.close();

    if (!parentHash.empty())
        commitContent << "parent: " << parentHash << "\n";

    auto now = chrono::system_clock::to_time_t(chrono::system_clock::now());
    string timeStr = ctime(&now);
    if (!timeStr.empty() && timeStr.back() == '\n')
        timeStr.pop_back();
    commitContent << "timestamp: " << timeStr << "\n";

    commitContent << "files:\n";

    string line;
    bool hasFiles = false;
    while (getline(indexFile, line)) {
        commitContent << line << "\n";
        hasFiles = true;
    }
    indexFile.close();

    if (!hasFiles) {
        cout << "No files staged. Commit aborted.\n";
        return;
    }

    string commitData = commitContent.str();
    string commitHash = simpleHash(commitData);

    // Ensure objects folder exists
    if (!fs::exists(".minigit/objects")) {
        cout << "ERROR: .minigit/objects folder does not exist.\n";
        return;
    }

    string commitPath = ".minigit/objects/" + commitHash;
    ofstream commitFile(commitPath);
    if (!commitFile) {
        cout << "ERROR: Could not open commit file for writing.\n";
        return;
    }

    commitFile << commitData;
    commitFile.close();

    // Update HEAD
    ofstream headOut(".minigit/HEAD");
    if (!headOut) {
        cout << "ERROR: Could not update HEAD.\n";
        return;
    }
    headOut << commitHash;
    headOut.close();

    // Clear index
    ofstream clearIndex(".minigit/index", ios::trunc);
    clearIndex.close();

    cout << "Committed successfully. Hash: " << commitHash << "\n";
}


void showLog() {
    ifstream headFile(".minigit/HEAD");
    string commitHash;
    getline(headFile, commitHash);
    headFile.close();

    // Sanitize any hidden characters (like \r or spaces)
    commitHash.erase(remove_if(commitHash.begin(), commitHash.end(), ::isspace), commitHash.end());

    if (commitHash.empty()) {
        cout << "No commits yet. Repository is empty.\n";
        return;
    }

    while (!commitHash.empty()) {
        string commitPath = ".minigit/objects/" + commitHash;
        ifstream commitFile(commitPath);
        if (!commitFile) {
            cout << "Error: Commit " << commitHash << " not found.\n";
            break;
        }

        cout << "Commit " << commitHash << ":\n";
        string line, parentHash;
        while (getline(commitFile, line)) {
            if (line.rfind("parent: ", 0) == 0) {
                parentHash = line.substr(8);
            } else {
                cout << line << "\n";
            }
        }

        commitFile.close();
        cout << "\n";

        // Clean parent hash too
        parentHash.erase(remove_if(parentHash.begin(), parentHash.end(), ::isspace), parentHash.end());
        commitHash = parentHash;
    }
}


void createBranch(const string& branchName) {
    // Read current HEAD
    ifstream headFile(".minigit/HEAD");
    string currentCommit;
    getline(headFile, currentCommit);
    headFile.close();

    if (currentCommit.empty()) {
        cout << "ERROR: No commits yet. Cannot create branch.\n";
        return;
    }

    // Make refs folder if not exists
    fs::create_directory(".minigit/refs");

    string branchPath = ".minigit/refs/" + branchName;
    if (fs::exists(branchPath)) {
        cout << "Branch '" << branchName << "' already exists.\n";
        return;
    }

    ofstream branchFile(branchPath);
    branchFile << currentCommit;
    branchFile.close();

    cout << "Branch '" << branchName << "' created at commit " << currentCommit << ".\n";
}
void checkoutBranch(const string& branchName) {
    string branchPath = ".minigit/refs/" + branchName;

    ifstream branchFile(branchPath);
    if (!branchFile) {
        cout << "ERROR: Branch '" << branchName << "' does not exist.\n";
        return;
    }

    string commitHash;
    getline(branchFile, commitHash);
    branchFile.close();

    if (commitHash.empty()) {
        cout << "ERROR: Branch '" << branchName << "' has no commits.\n";
        return;
    }

    
    string commitPath = ".minigit/objects/" + commitHash;
    ifstream commitFile(commitPath);
    if (!commitFile) {
        cout << "ERROR: Commit object not found.\n";
        cout << "Tried to open: " << commitPath << "\n";  // 🔍 Help debug
        return;
    }

    string line;
    bool inFileSection = false;

    while (getline(commitFile, line)) {
        if (line == "files:") {
            inFileSection = true;
            continue;
        }

        if (inFileSection && !line.empty()) {
            stringstream ss(line);
            string filename, blobHash;
            ss >> filename >> blobHash;

            string blobPath = ".minigit/objects/" + blobHash;
            ifstream blob(blobPath);
            if (!blob) {
                cout << "ERROR: Blob for file '" << filename << "' is missing.\n";
                continue;
            }

            ofstream outFile(filename);
            outFile << blob.rdbuf();
            outFile.close();
        }
    }

    commitFile.close();

    
    ofstream headOut(".minigit/HEAD");
    headOut << commitHash;
    headOut.close();

    cout << "Checked out branch '" << branchName << "'.\n";
}
void mergeBranch(const string& branchName) {
    string branchRef = ".minigit/refs/" + branchName;

    if (!fs::exists(branchRef)) {
        cout << "ERROR: Branch '" << branchName << "' does not exist.\n";
        return;
    }

    // Get commit hash from the branch
    ifstream branchFile(branchRef);
    string otherCommitHash;
    getline(branchFile, otherCommitHash);
    branchFile.close();

    if (otherCommitHash.empty()) {
        cout << "ERROR: Branch '" << branchName << "' has no commits.\n";
        return;
    }

    string otherCommitPath = ".minigit/objects/" + otherCommitHash;
    ifstream commitFile(otherCommitPath);
    if (!commitFile) {
        cout << "ERROR: Commit object from '" << branchName << "' not found.\n";
        return;
    }

    cout << "Merging branch '" << branchName << "'...\n";

    string line;
    bool inFileSection = false;

    // Clear current index (optional: allow combining)
    ofstream(".minigit/index", ios::trunc).close();

    // Read each file listed in the commit and stage it
    while (getline(commitFile, line)) {
        if (line == "files:") {
            inFileSection = true;
            continue;
        }

        if (inFileSection && !line.empty()) {
            stringstream ss(line);
            string filename, blobHash;
            ss >> filename >> blobHash;

            string blobPath = ".minigit/objects/" + blobHash;
            ifstream blob(blobPath);
            if (!blob) {
                cout << "ERROR: Blob '" << blobHash << "' for file '" << filename << "' not found.\n";
                continue;
            }

            ofstream outFile(filename);
            outFile << blob.rdbuf();
            outFile.close();

            // Add to index
            ofstream indexFile(".minigit/index", ios::app);
            indexFile << filename << " " << blobHash << "\n";
            indexFile.close();
        }
    }

    commitFile.close();

    // Auto commit merge
    commit("Merged branch " + branchName);
}
void diffCommits(const string& hash1, const string& hash2) {
    string path1 = ".minigit/objects/" + hash1;
    string path2 = ".minigit/objects/" + hash2;

    ifstream f1(path1), f2(path2);
    if (!f1 || !f2) {
        cout << "ERROR: One or both commits not found.\n";
        return;
    }

    map<string, string> files1;
    map<string, string> files2;

    string line;
    bool inFiles = false;

    // Read files from first commit
    while (getline(f1, line)) {
        if (line == "files:") {
            inFiles = true;
            continue;
        }
        if (inFiles && !line.empty()) {
            stringstream ss(line);
            string filename, hash;
            ss >> filename >> hash;
            files1[filename] = hash;
        }
    }

    inFiles = false;
    // Read files from second commit
    while (getline(f2, line)) {
        if (line == "files:") {
            inFiles = true;
            continue;
        }
        if (inFiles && !line.empty()) {
            stringstream ss(line);
            string filename, hash;
            ss >> filename >> hash;
            files2[filename] = hash;
        }
    }

    // Collect all filenames appearing in either commit
    set<string> allFiles;
    for (const auto& [f, _] : files1) allFiles.insert(f);
    for (const auto& [f, _] : files2) allFiles.insert(f);

    for (const string& file : allFiles) {
        cout << "File: " << file << "\n";

        bool inFirst = files1.count(file) > 0;
        bool inSecond = files2.count(file) > 0;

        // Helper lambda to read blob content or empty if missing
        auto readBlob = [](const string& blobHash) -> string {
            if (blobHash.empty()) return "";
            ifstream blobFile(".minigit/objects/" + blobHash);
            if (!blobFile) return "";
            stringstream buffer;
            buffer << blobFile.rdbuf();
            return buffer.str();
        };

        string content1 = inFirst ? readBlob(files1[file]) : "";
        string content2 = inSecond ? readBlob(files2[file]) : "";

        // Case 1: File only in first commit (deleted in second)
        if (inFirst && !inSecond) {
            // Show entire content1 as removed
            stringstream ss(content1);
            string line;
            while (getline(ss, line)) {
                cout << "  - " << line << "\n";
            }
            if (content1.empty()) cout << "  - (empty file)\n";
            cout << "\n";
            continue;
        }

        // Case 2: File only in second commit (added)
        if (!inFirst && inSecond) {
            // Show entire content2 as added
            stringstream ss(content2);
            string line;
            while (getline(ss, line)) {
                cout << "  + " << line << "\n";
            }
            if (content2.empty()) cout << "  + (empty file)\n";
            cout << "\n";
            continue;
        }

        // Case 3: File in both commits - compare content line-by-line
        if (content1 == content2) {
            cout << "  No changes.\n\n";
            continue;
        }

        vector<string> lines1, lines2;
        {
            stringstream ss1(content1), ss2(content2);
            string temp;
            while (getline(ss1, temp)) lines1.push_back(temp);
            while (getline(ss2, temp)) lines2.push_back(temp);
        }

        size_t maxLines = max(lines1.size(), lines2.size());
        for (size_t i = 0; i < maxLines; ++i) {
            string l1 = (i < lines1.size()) ? lines1[i] : "";
            string l2 = (i < lines2.size()) ? lines2[i] : "";

            if (l1 != l2) {
                cout << "  - " << l1 << "\n";
                cout << "  + " << l2 << "\n";
            }
        }
        cout << "\n";
    }
}







// Main command loop
int main() {
    string command;
    cout << "MiniGit started.\n";

    while (true) {
        cout << "minigit> ";
        getline(cin, command);

        if (!command.empty() && command.back() == '\r')
            command.pop_back();

        if (command == "init") {
            init();
        } else if (command.rfind("add ", 0) == 0) {
            addFile(command.substr(4));
        } else if (command.rfind("commit -m ", 0) == 0) {
            commit(command.substr(10));
        } else if (command == "log") {
            showLog();
        } else if (command.rfind("branch ", 0) == 0) {
            createBranch(command.substr(7));
        } else if (command.rfind("checkout ", 0) == 0) {
            checkoutBranch(command.substr(9));
        } else if (command.rfind("merge ", 0) == 0) {
            mergeBranch(command.substr(6));
        } else if (command.rfind("diff ", 0) == 0) {
            string args = command.substr(5);
            size_t space = args.find(' ');
            if (space == string::npos) {
                cout << "Usage: diff <commit1> <commit2>\n";
            } else {
                string hash1 = args.substr(0, space);
                string hash2 = args.substr(space + 1);
                diffCommits(hash1, hash2);
            }
        } else if (command == "exit") {
            break;
        } else {
            cout << "Unknown command.\n";
        }
    }

    return 0;
}
