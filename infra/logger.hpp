#pragma once
#include <fstream>


struct Logger {
  std::ofstream logfile;
  std::string filePath;
  Logger () { filePath = ""; }
  void open (auto && path) { 
    logfile.open(path); 
    filePath = path;
  }
  Logger (auto && path) { open(path); }
  
  
  void close() { 
    if (filePath != "") {
      logfile.close(); 
      std::cout << "Log has been saved to " + filePath + "\n";  
    } 
  }
  
  
  void operator() ( auto && s ) {
    std::cout << s; 
    if (filePath != "") logfile << s;  
  } 
};


// Logger logger;




