#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <chrono>
#include <algorithm>


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






// Main command loop
int main() {
    string command;
    cout << "MiniGit started.\n";

    while (true) {
        cout << "minigit> ";
        getline(cin, command);

        // Fix for carriage return (common on Windows)
        if (!command.empty() && command.back() == '\r')
            command.pop_back();

        if (command == "init") {
            init();
        } else if (command.rfind("add ", 0) == 0) {
            string filename = command.substr(4);
            addFile(filename);
        } else if (command.rfind("commit -m ", 0) == 0) {
            string message = command.substr(10);
            commit(message);
        } else if (command == "log") {
            showLog();
        } else if (command.rfind("branch ", 0) == 0) {
            string branchName = command.substr(7);
            createBranch(branchName);
        } else if (command.rfind("checkout ", 0) == 0) {
            string branchName = command.substr(9);
            checkoutBranch(branchName);
        } else if (command.rfind("merge ", 0) == 0) {
            string branchName = command.substr(6);
            mergeBranch(branchName);
        } else if (command == "exit") {
            break;
        } else {
            cout << "Unknown command.\n";
        }
    }

    return 0;
}
