// Test adapter only: compile and execute MASTER's unchanged framing parser.
#include <iostream>
#include <iterator>
#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif
#include "../../firmware/01_MASTER/TubaTech_Integrated_Master/api_policy.h"

int main() {
#ifdef _WIN32
  _setmode(_fileno(stdin), _O_BINARY);
#endif
  const std::string bytes((std::istreambuf_iterator<char>(std::cin)),
                          std::istreambuf_iterator<char>());
  local::HttpFrame frame;
  for (char byte : bytes) frame.feed(byte);
  std::cout << "{\"accepted\":" << (frame.complete && !frame.failed ? "true" : "false")
            << ",\"method\":" << local::quote(frame.request.method)
            << ",\"path\":" << local::quote(frame.request.path)
            << ",\"body\":" << local::quote(frame.request.body) << "}";
}
