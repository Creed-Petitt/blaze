#include <cstdint>
#include <cstddef>
#include <string>
#include <blaze/json.h>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size) {
    if (Size == 0) return 0;
    
    // Construct string from fuzz data
    std::string input(reinterpret_cast<const char*>(Data), Size);
    
    try {
        // Target: Json::parse (constructor)
        // We don't care about the output, just that it doesn't crash
        blaze::Json j = blaze::Json(input);
        (void)j.dump(); // Force serialization too
    } catch (...) {
        // Parsing errors are expected and safe
    }
    
    return 0;
}
