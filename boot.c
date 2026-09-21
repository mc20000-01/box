#include <unistd.h>

int main() {
    write(STDOUT_FILENO, "booting boxed-os\n", 17);
    return 0;
}