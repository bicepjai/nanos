// op_codes.h, 159 P7
// operation codes for OS services, e.g., FileSys() and device drivers
//
// here are example of codes covering what you need, not all used

#ifndef _OP_CODES_H_
#define _OP_CODES_H_

// -------------------- General Code ----------------------
#define OK                0
#define NOT_OK           -1
#define NOT_USED         -1

// -------------------- MODE Related ----------------------
#define ECHO_ON          77
#define ECHO_OFF         88

// ------------------ FileSys Result ----------------------
#define UNKNOWN_OP_CODE  -99
#define NOT_FOUND        -21
#define NO_MORE_FD       -12 // all file descriptors in use
#define END_OF_FILE      -13 // no more content
#define BAD_PARAM        -06 // some parameter was invalid
#define BUFF_TOO_SMALL   -17 // your buffer too small

// ------------------- FileSys Service --------------------
#define RDONLY 0
#define WRONLY 1
#define RDWR   2

#define STAT_NAME 21
// message passing:
//    supply filename string (NUL terminated) in message
//    return file status structure (overwrites the filename string)

#define OPEN_NAME 22
// message passing:
//    supply filename (NUL terminated)
//    supply operation flag such as RDONLY
//    return file descriptor if successful, -1 if failure

#define CLOSE_FD 23
// message passing:
//    supply file descriptor

#define READ_FD 24
// message passing:
//    supply file descriptor
//    supply buffer in a msg
//    return number of bytes read
//    return content read in a msg

#define SEEK_FD 26
// message passing:
//    supply file descriptor
//    supply offset (a +/- value)
//    supply whence (base: head, tail, or the last position)
//    return absolute offset (head+offset)

#endif

