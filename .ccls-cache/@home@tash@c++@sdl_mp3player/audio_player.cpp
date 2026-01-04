#include <iostream>
#include <string.h>
#include <fstream>
#include <vector>

std::string mp3FilePath = "mp3/01 Ruin.mp3"; 

void getAudioType(){
    // set a delimeter of "."
    // split the string into two => [name, file_ext]
    // get type of decoding by going decodeTyep[file_ext]
}

void openFile(std::string path) {
    std::ifstream output(path);
    std::vector<int> buffer;
    //int data[1024];
    int buf_len = 1024;
    int diviser = 2;
    int marker =0;
    int indx = 0;
    int stop_value = 1024 * 3;
    for (indx=0;indx<stop_value; indx+=buf_len) {
        marker+=buf_len;
        //buffer[indx:buf_len];
        buffer.push_back([indx:marker])
        //marker+=buf_len;
    }
    //std::string line;
    //while (std::getline(output, line)) {
      //      std::cout << line << std::endl;
        //}
        //
}

int main(){
    std::cout << "Hello";
    openFile(mp3FilePath);
    return 0;
}
