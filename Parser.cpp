#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>

using namespace std;

void translateRule(const vector<string>& rule) {
    // Determine if it's a disjunctive rule or a choice rule
    bool isDisjunctive = (rule[1] == "0");
    string ruleType = isDisjunctive ? "disjunction" : "choice";
    if(ruleType=="choice"){
        cerr<<"WARNING!\nChoice rule are not acceptable\n";
        exit(1);
    }

    // Extract the head elements
    int numHeadElements = stoi(rule[2]);
    vector<string> headElements(rule.begin() + 3, rule.begin() + 3 + numHeadElements);

    // Extract the body elements
    vector<string> bodyElements;
    int index = 3 + numHeadElements + 1; // Skip past the head elements and the "0" separator
    int numBodyElements = stoi(rule[index]);
    index++;
    for (int i = 0; i < numBodyElements; i++) {
        bodyElements.push_back(rule[index + i]);
    }

    // Print the rule in a human-readable format
    cout << "Rule type: " << ruleType << endl;
    cout << "Head: ";
    for (const string& element : headElements) {
        cout << element << " ";
    }
    cout << endl;

    if (!bodyElements.empty()) {
        cout << "Body: ";
        for (const string& element : bodyElements) {
            cout << element << " ";
        }
        cout << endl;
    }

    cout << endl;
}

// ... (rest of the code remains the same)

int main() {
    ifstream file("output.txt");
    if (!file) {
        cerr << "Failed to open file" << endl;
        return 1;
    }

    string line;
    while (getline(file, line)) {
        if (line.empty() || line == "0") {
            // Skip empty lines and the terminator "0"
            continue;
        }

        istringstream iss(line);
        vector<string> rule;
        string token;
        while (iss >> token) {
            rule.push_back(token);
        }

        // Check if the line starts with "1" (indicating a rule)
        if (rule[0] == "1") {
            translateRule(rule);
        } else {
            // Handle other lines (e.g., atoms, comments, etc.) if needed
            // ...
        }
    }

    file.close();
    return 0;
}