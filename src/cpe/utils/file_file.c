#include <assert.h>
#include <string.h>
#include "file_internal.h"
#include "cpe/pal/pal_stdio.h"
#include "cpe/utils/stream_mem.h"
#include "cpe/utils/stream_buffer.h"

ssize_t file_write_from_buf(const char * file, const void * buf, size_t size, error_monitor_t em) {
    ssize_t totalSize;
    FILE * fp;
    
    fp = file_stream_open(file, "w", em);
    if (fp == NULL) return -1;

    totalSize = file_stream_write_from_buf(fp, buf, size, em);

    file_stream_close(fp, em);

    return totalSize;
}

ssize_t file_write_from_str(const char * file, const char * str, error_monitor_t em) {
    return file_write_from_buf(file, str, strlen(str), em);
}

ssize_t file_write_from_stream(const char * file, read_stream_t stream, error_monitor_t em) {
    ssize_t totalSize;
    FILE * fp;

    fp = file_stream_open(file, "w", em);
    if (fp == NULL) return -1;

    totalSize = file_stream_write_from_stream(fp, stream, em);

    file_stream_close(fp, em);

    return totalSize;
}

ssize_t file_append_from_buf(const char * file, const void * buf, size_t size, error_monitor_t em) {
    ssize_t totalSize;
    FILE * fp;

    fp = file_stream_open(file, "a", em);
    if (fp == NULL) return -1;

    totalSize = file_stream_write_from_buf(fp, buf, size, em);

    file_stream_close(fp, em);

    return totalSize;
}

ssize_t file_append_from_str(const char * file, const char * str, error_monitor_t em) {
    return file_append_from_buf(file, str, strlen(str), em);
}

ssize_t file_append_from_stream(const char * file, read_stream_t stream, error_monitor_t em) {
    FILE * fp;
    ssize_t totalSize;

    fp = file_stream_open(file, "a", em);
    if (fp == NULL) return -1;

    totalSize = file_stream_write_from_stream(fp, stream, em);

    file_stream_close(fp, em);

    return totalSize;
}

ssize_t file_load_to_buffer(mem_buffer_t buffer, const char * file, error_monitor_t em) {
    struct write_stream_buffer stream = CPE_WRITE_STREAM_BUFFER_INITIALIZER(buffer);
    return file_load_to_stream((write_stream_t)&stream, file, em);
}

ssize_t file_load_to_stream(write_stream_t stream, const char * file, error_monitor_t em) {
    FILE * fp;
    ssize_t totalSize;

    fp = file_stream_open(file, "r", em);
    if (fp == NULL) return -1;

    totalSize = file_stream_load_to_stream(stream, fp, em);

    if (!feof(fp)) {
        totalSize = -1;
    }

    file_stream_close(fp, em);
    return totalSize;
}

ssize_t file_stream_write_from_buf(FILE * fp, const void * buf, size_t size, error_monitor_t em) {
    ssize_t totalSize;
    size_t writeSize;

    totalSize = 0;
    while((writeSize = fwrite(buf, 1, size, fp)) > 0) {
        size -= writeSize;
        totalSize += writeSize;
    }

    if (ferror(fp)) {
        totalSize = -1;
    }

    return totalSize;
}

ssize_t file_stream_write_from_str(FILE * fp, const char * str, error_monitor_t em) {
    return file_stream_write_from_buf(fp, str, strlen(str), em);
}

ssize_t file_stream_write_from_stream(FILE * fp, read_stream_t stream, error_monitor_t em) {
    ssize_t totalSize;
    size_t writeSize;
    size_t writeOkSize;
    size_t size;
    char buf[128];

    totalSize = 0;
    while((size = stream_read(stream, buf, 128)) > 0) {
        writeOkSize = 0;
        while(size > writeOkSize
              && (writeSize = fwrite(buf + writeOkSize, 1, size - writeOkSize, fp)) > 0)
        {
            writeOkSize += writeSize;
        }

        totalSize += writeOkSize;

        if (writeOkSize < size) break;
    }

    if (ferror(fp)) {
        totalSize = -1;
    }

    return totalSize;
}

ssize_t file_stream_load_to_buf(char * buf, size_t size, FILE * fp, error_monitor_t em) {
    struct write_stream_mem stream = CPE_WRITE_STREAM_MEM_INITIALIZER(buf, size);
    return file_stream_load_to_stream((write_stream_t)&stream, fp, em);
}

ssize_t file_stream_load_to_buffer(mem_buffer_t buffer, FILE * fp, error_monitor_t em) {
    struct write_stream_buffer stream = CPE_WRITE_STREAM_BUFFER_INITIALIZER(buffer);
    return file_stream_load_to_stream((write_stream_t)&stream, fp, em);
}

ssize_t file_stream_load_to_stream(write_stream_t stream, FILE * fp, error_monitor_t em) {
    size_t writeSize;
    size_t writeOkSize;
    size_t size;
    ssize_t totalSize;
    char buf[128];

    totalSize = 0;
    while((size = fread(buf, 1, 128, fp)) > 0) {
        writeOkSize = 0;
        while(size > writeOkSize
              && (writeSize = stream_write(stream, buf + writeOkSize, size - writeOkSize)) > 0)
        {
            writeOkSize += writeSize;
        }

        totalSize += writeOkSize;
        if (writeOkSize < size) break;
    }

    if (ferror(fp)) {
        totalSize = -1;
    }

    return totalSize;
}

int file_exist(const char * path, error_monitor_t em) {
    struct stat buffer;
    int status;
    status = inode_stat_by_path(path, &buffer, ENOENT, em);
    if (status != 0) {
        return 0;
    }

    return S_ISREG(buffer.st_mode);
}

ssize_t file_size(const char * path, error_monitor_t em) {
    struct stat buffer;
    int status;
    status = inode_stat_by_path(path, &buffer, 0, em);
    if (status != 0) {
        return -1;
    }

    if (!S_ISREG(buffer.st_mode)) {
        CPE_ERROR(em, "%s is not file.", path);
        return -1;
    }

    return (ssize_t)buffer.st_size;
}

ssize_t file_stream_size(FILE * fp, error_monitor_t em) {
    struct stat buffer;
    int status;
    status = inode_stat_by_fileno(fileno(fp), &buffer, 0, em);
    if (status != 0) {
        return -1;
    }

    return (ssize_t)buffer.st_size;
}

const char * dir_name(const char * input, mem_buffer_t tbuf) {
    return dir_name_ex(input, 1, tbuf);
}

const char * dir_name_ex(const char * input, int level, mem_buffer_t tbuf) {
    int len;
    assert(level > 0);

    if (input == NULL) return NULL;

    len = strlen(input);

    while(len > 0) {
        char c = input[--len];
        if (c == '/' || c == '\\') {
            --level;
            if (level == 0) {
                char * r = (char *)mem_buffer_alloc(tbuf, len + 1);
                if (len > 1) { memcpy(r, input, len); }
                r[len] = 0; 
                return r;
            }
        }
    }

    return NULL;
}

const char * file_name_suffix(const char * input) {
    int len;

    if (input == NULL) return NULL;

    len = strlen(input);

    while(len > 0) {
        char c = input[--len];
        if (c == '.') return input + len + 1;
        if (c == '/' || c == '\\') return "";
    }

    return input;
}

const char * file_name_no_dir(const char * input) {
    int len;

    if (input == NULL) return NULL;

    len = strlen(input);

    while(len > 0) {
        char c = input[--len];
        if (c == '/' || c == '\\') return input + len + 1;
    }

    return input;
}

const char *
file_name_base(const char * input, mem_buffer_t tbuf) {
    int len;
    int endPos;
    int beginPos;
    int pos;

    if (input == NULL) return NULL;

    len = strlen(input);
    
    endPos = len + 1;
    beginPos = 0;

    pos = len + 1;
    while(pos > 0) {
        char c = input[--pos];
        if (c == '.') {
            if (endPos == len + 1) {
                endPos = pos;
            }
        }

        if (c == '/' || c == '\\') {
            beginPos = pos + 1;
            break;
        }
    }

    if (endPos == len + 1) {
        return input + beginPos;
    }
    else {
        char * r;
        int resultLen = endPos - beginPos;
        if (resultLen <= 0) return "";

        assert(beginPos < endPos);
        r = (char *)mem_buffer_alloc(tbuf, resultLen + 1);
        memcpy(r, input + beginPos, resultLen);
        r[resultLen] = 0;
        return r;
    }
}

const char * file_name_append_base(mem_buffer_t tbuf, const char * input) {
    if (mem_buffer_size(tbuf) == 0) {
        return file_name_base(input, tbuf);
    }
    else {
        mem_buffer_set_size(tbuf, mem_buffer_size(tbuf) - 1);
        return file_name_base(input, tbuf);
    }
}
