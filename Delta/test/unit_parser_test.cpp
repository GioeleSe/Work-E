// Host-side unit tests for JSON parsing logic used by `communication.cpp`.
// These tests don't require ESP32 hardware; they validate that example
// JSON messages contain the expected fields and types.

#include <ArduinoJson.h>
#include <iostream>
#include <string>
#include <fstream>
#include <vector>

using namespace std;

static int tests_passed = 0;
static int tests_failed = 0;

static void expect(bool cond, const char* msg){
  if(cond){ tests_passed++; cout << "PASS: " << msg << "\n"; }
  else { tests_failed++; cout << "FAIL: " << msg << "\n"; }
}

void test_emergency_stop(){
  // not used when reading from file
}

void test_move(){
  // not used when reading from file
}

void test_malformed(){
  // not used when reading from file
}

int main(){
  cout << "Running parser unit tests (from test/jsons.txt)...\n";
  // read the jsons.txt file
  std::ifstream f("test/jsons.txt");
  if(!f){
    cout << "Could not open test/jsons.txt\n";
    return 2;
  }
  string line; vector<string> lines;
  while(std::getline(f, line)) lines.push_back(line);
  // strip code fences and remove // comments, then extract JSON objects by brace matching
  string accum;
  vector<string> objects;
  for(string &ln : lines){
    string s = ln;
    // skip fences
    if(s.find("```")!=string::npos) continue;
    // remove inline // comments
    size_t cpos = s.find("//");
    if(cpos!=string::npos) s = s.substr(0,cpos);
    // append
    for(char ch: s) accum.push_back(ch);
    accum.push_back('\n');
  }
  // extract JSON objects
  int depth = 0; string cur;
  for(char ch: accum){
    if(ch == '{'){
      depth++;
      cur.push_back(ch);
    } else if(ch == '}'){
      cur.push_back(ch);
      depth--;
      if(depth==0){
        objects.push_back(cur);
        cur.clear();
      }
    } else {
      if(depth>0) cur.push_back(ch);
    }
  }

  if(objects.empty()){
    cout << "No JSON objects found in test/jsons.txt\n";
    return 2;
  }

  int idx = 0;
  for(const string &obj : objects){
    idx++;
    cout << "--- Object "<<idx<<" ---\n";
    StaticJsonDocument<2048> doc;
    auto err = deserializeJson(doc, obj.c_str());
    if(err){
      cout << "PARSE FAIL: "<<err.c_str()<<"\n";
      tests_failed++;
      continue;
    }
    // basic checks
    bool ok = true;
    const char* protocol = doc["protocol"];
    if(!protocol || strcmp(protocol,"robot-net/1.0")!=0){ ok=false; cout<<"protocol mismatch\n"; }
    if(!doc["payload"].is<JsonObject>()){ ok=false; cout<<"missing payload\n"; }
    if(ok){ tests_passed++; cout<<"PASS\n"; } else { tests_failed++; cout<<"FAIL\n"; }
  }

  cout << "\nResults: " << tests_passed << " passed, " << tests_failed << " failed.\n";
  return tests_failed == 0 ? 0 : 1;
}
