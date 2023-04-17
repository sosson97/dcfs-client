// base test
// C-style file read/write test
// Use system calls directly

// C++ headers
#include <string>
#include <filesystem>


// C headers
#include <cstdio>
#include <cstdlib>
#include <sys/time.h>

// system call
#include <fcntl.h>
#include <unistd.h>

namespace fs = std::filesystem;

#define FILE_SiZE (4 * 1024 * 1024)

static double start_time[1];

static double get_time() {
        struct timeval tv;
        gettimeofday(&tv, 0);
        return tv.tv_sec *1e6 + tv.tv_usec;
}

void timer_start(int local) {
        start_time[local] = get_time();
}

double timer_stop(int local) {
        return get_time() - start_time[local];
}


class Logger {
public:
    Logger() { header_ = ""; }
    void set_header(std::string header) { header_ = header; }
    void log(std::string msg) { printf("[TEST]%s%s\n", header_.c_str(), msg.c_str()); }
    void error(std::string msg) { printf("[ERROR]%s%s\n", header_.c_str(), msg.c_str()); }
    void report(std::string msg) { printf("[REPORT]%s%s\n", header_.c_str(), msg.c_str()); }
private:
    std::string header_;
};



class Tester {
public:
    void set_dir(std::string dirname) { dir_ = dirname; }
    void run();

private:
    void init_test();

    void Test1();
    void Test2();
    void Test3();

    char *buf;
    char *ans;
    Logger lg_;
    fs::path dir_;
    fs::path file_; 
};

static void init_buf(char *buf, int size) {
    for (int i = 0; i < size; i++) {
        buf[i] = rand() % 256;
    }
}

// return negative if they are equal
// return the index of the first different byte
static int compare_buf(char *buf1, char *buf2, int size) {
    for (int i = 0; i < size; i++) {
        if (buf1[i] != buf2[i]) {
            return i;
        }
    }
    return -1;
}

void Tester::run() {
    init_test();

    lg_.log("Start test");
    Test1();
    Test2();
    Test3();

    lg_.set_header(""); 
    lg_.log("Test successfully finished");
}

void Tester::init_test() 
{
    file_ = dir_;
    file_ += "/test.txt";

    buf = new char[FILE_SiZE];
    ans = new char[FILE_SiZE];
}

void Tester::Test1() {
    int ret;
    int fd;
    double time;
    lg_.set_header("[Test1] ");

    timer_start(0);
    fd = open(file_.c_str(), O_RDWR | O_CREAT, 0644);
    if (fd < 0) {
        lg_.error("Failed to open file");
        return;
    }
    time = timer_stop(0);
    lg_.report("Open file time: " + std::to_string(time) + " us");

    init_buf(ans, FILE_SiZE);

    timer_start(0);
    ret = write(fd, ans, FILE_SiZE);
    if (ret != FILE_SiZE) {
        lg_.error("Failed to write file");
        return; 
    }
    time = timer_stop(0);
    lg_.report("Write file time: " + std::to_string(time) + " us");


    ret = lseek(fd, 0, SEEK_SET);
    if (ret != 0) {
        lg_.error("Failed to seek file");
        return;
    }

    timer_start(0);
    ret = read(fd, buf, FILE_SiZE);
    if (ret != FILE_SiZE) {
        lg_.error("Failed to read file");
        return;
    }
    time = timer_stop(0);
    lg_.report("Read file time: " + std::to_string(time) + " us");

    ret = compare_buf(ans, buf, FILE_SiZE);
    if (ret != -1) {
        lg_.error("Failed to pass correctness in Test1");
        lg_.error("First different byte at " + std::to_string(ret));
        return;
    }
    lg_.report("Correctness passed");

    timer_start(0);
    ret = close(fd);
    if (ret != 0) {
        lg_.error("Failed to close file");
        return;
    }
    time = timer_stop(0);
    lg_.report("Close file time: " + std::to_string(time) + " us");
    lg_.report("================Test1 finished successfully==============");
}

void Tester::Test2() {
    int ret;
    int fd;
    double time;
    lg_.set_header("[Test2] ");

    timer_start(0);
    fd = open(file_.c_str(), O_RDWR | O_CREAT, 0644);
    if (fd < 0) {
        lg_.error("Failed to open file");
        return;
    }
    time = timer_stop(0);
    lg_.report("Open file time: " + std::to_string(time) + " us");

    ret = lseek(fd, 0, SEEK_SET);
    if (ret != 0) {
        lg_.error("Failed to seek file");
        return;
    }

    timer_start(0);
    ret = read(fd, buf, FILE_SiZE);
    if (ret != FILE_SiZE) {
        lg_.error("Failed to read file");
        return;
    }
    time = timer_stop(0);
    lg_.report("Read file time: " + std::to_string(time) + " us");

    ret = compare_buf(ans, buf, FILE_SiZE);
    if (ret != -1) {
        lg_.error("Failed to pass correctness in Test1");
        lg_.error("First different byte: " + std::to_string(ret));
        return;
    }
    lg_.report("Correctness passed");

    timer_start(0);
    ret = close(fd);
    if (ret != 0) {
        lg_.error("Failed to close file");
        return;
    }
    time = timer_stop(0);
    lg_.report("Close file time: " + std::to_string(time) + " us");

    lg_.report("================Test2 finished successfully==============");
}

// not implemetned yet
// overwrite test
// will test overwrite spanning across multiple blocks
void Tester::Test3() {
    lg_.set_header("[Test3] ");
    lg_.log("Not implemented yet");
    return;
}

int main (int argc, char **argv) {  
    if (argc != 2) {
        printf("Usage: %s <test_dir>\n", argv[0]);
        exit(1);
    }
    
    auto t = Tester();
    t.set_dir(argv[1]);
    
    t.run();

    return 0;
}