# MiniGit Project Documentation

## Overview
MiniGit is a simplified version control system implemented in C++. It allows users to initialize a repository, stage files, commit changes with messages, create branches, switch branches, view commit logs, merge branches, and view differences between commits. The system mimics basic Git functionalities with simple hashing and object storage in a `.minigit` directory.

## Features
- **init**: Initialize a new MiniGit repository.
- **add <filename>**: Stage a file to be committed.
- **commit -m "message"**: Commit staged files with a commit message.
- **log**: Display the commit history starting from the latest commit.
- **branch <branchname>**: Create a new branch from the current HEAD commit.
- **checkout <branchname>**: Switch to an existing branch and update working directory files.
- **merge <branchname>**: Merge changes from another branch into the current branch.
- **diff <commitHash1> <commitHash2>**: Show line-by-line differences between two commits.
- **exit**: Exit the MiniGit command prompt.

## File Structure
- `.minigit/` - Root directory for MiniGit metadata.
  - `objects/` - Stores file blobs and commit objects named by their hash.
  - `refs/` - Stores branch references, each file named by branch name containing commit hash.
  - `index` - Staging area, tracks files to be committed.
  - `HEAD` - Points to the current commit hash or branch reference.

## Commands Description

### init
Creates the `.minigit` directory with necessary subfolders and files:
- `.minigit/objects/`
- `.minigit/refs/`
- `.minigit/index` (empty file)
- `.minigit/HEAD` (empty or points to initial commit)

### add <filename>
Reads the contents of `<filename>`, computes a hash, saves the contents to `.minigit/objects/<hash>`, and updates `.minigit/index` with the filename and hash.

### commit -m "message"
Creates a commit object containing:
- Commit message
- Parent commit hash (if exists)
- Timestamp
- List of files staged with their hashes

Saves this commit in `.minigit/objects/<commit_hash>` and updates `HEAD` to point to this new commit. Clears the index.

### log
Reads the commit hash from `HEAD` and traverses parent commits, displaying commit messages, timestamps, and files until no parent commit is found.

### branch <branchname>
Creates a file `.minigit/refs/<branchname>` pointing to the current commit hash. Prevents duplicate branch names.

### checkout <branchname>
Reads the commit hash from `.minigit/refs/<branchname>`, reads commit files, and restores staged files to the working directory. Updates `HEAD` to the commit hash.

### merge <branchname>
Merges the commit pointed to by `<branchname>` into the current branch’s commit, updating files and creating a new commit. Handles basic file merging but does not resolve complex conflicts.

### diff <commitHash1> <commitHash2>
Displays line-by-line differences between files in two commits.

Example output:
```
File: hello.txt
  - Old line removed
  + New line added

File: notes.txt
  No changes.
```
- Lines with `-` existed only in the first commit.
- Lines with `+` were added in the second commit.
- Identifies exact line-level modifications for each file that changed.

### exit
Exits the MiniGit interactive prompt.

## Usage Example
```sh
minigit> init
Initialized empty MiniGit repository in .minigit/

minigit> add hello.txt
Added 'hello.txt' to staging area.

minigit> commit -m "Initial commit"
Committed successfully. Hash: 1234567890

minigit> log
Commit 1234567890:
message: Initial commit
timestamp: Thu Jun 19 20:00:00 2025
files:
hello.txt 9876543210

minigit> branch dev
Branch 'dev' created.

minigit> checkout dev
Checked out branch 'dev'.

minigit> merge master
Merged 'master' into current branch.

minigit> diff 1234567890 2345678901
File: hello.txt
  - Hello
  + Hello, world!

minigit> exit
```

## Notes
- The hash function used is a simple placeholder (`std::hash`) and is not collision-resistant.
- The merge function is basic and may not handle conflicts correctly.
- The diff feature uses simple line comparison and does not implement full LCS (longest common subsequence) diffing.
- This project is for educational purposes to understand the fundamentals of version control.

## Setup and Build
Compile using:
```sh
g++ -std=c++17 main.cpp -o minigit.exe
```

Run using:
```sh
./minigit.exe
```

## Author
Created as an educational project for understanding version control system implementation in C++.
