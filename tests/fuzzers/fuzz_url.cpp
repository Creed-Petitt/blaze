#include <cstdint>
#include <cstddef>
#include <string>
#include <blaze/util/string.h>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size) {
    if (Size == 0) return 0;
    
    // Construct string from fuzz data
    std::string input(reinterpret_cast<const char*>(Data), Size);
    
    // Target: url_decode
    // We don't care about the output, just that it doesn't crash (ASan/UBSan will catch issues)
    std::string output = blaze::util::url_decode(input);
    
    return 0;
}
