#include <zlib.h>

std::string decompressGzip(const std::string& compressedData)
{
    z_stream zs;
    memset(&zs, 0, sizeof(zs));

    if (inflateInit2(&zs, 16 + MAX_WBITS) != Z_OK)
        throw(std::runtime_error("inflateInit failed while decompressing."));

    zs.next_in = (Bytef*)compressedData.data();
    zs.avail_in = compressedData.size();

    int ret;
    char outbuffer[32768];
    std::string decompressedData;

    do {
        zs.next_out = reinterpret_cast<Bytef*>(outbuffer);
        zs.avail_out = sizeof(outbuffer);

        ret = inflate(&zs, 0);

        if (decompressedData.size() < zs.total_out) {
            decompressedData.append(outbuffer, zs.total_out - decompressedData.size());
        }

    } while (ret == Z_OK);

    inflateEnd(&zs);

    if (ret != Z_STREAM_END) {
        std::ostringstream oss;
        oss << "Exception during zlib decompression: (" << ret << ") " << zs.msg;
        throw(std::runtime_error(oss.str()));
    }

    return decompressedData;
}