#ifndef CONSTANTS_H
#define CONSTANTS_H

#define UI 1
#define REGISTRATION_TEST 0

#define OK_MESSAGE "OKY\r\n"
#define ERR_MESSAGE "ERR\r\n"
#define LOGIN_MESSAGE "LIN\r\n"
#define LOGOUT_MESSAGE "LOG\r\n"
#define AVATAR_MESSAGE "AVT\r\n"
#define REGISTRATION_MESSAGE "REG\r\n"
#define REQUEST_FILE_MESSAGE "RFL\r\n"
#define LIST_OF_FILE "LSF\r\n"
#define LIST_OF_USERS "USR\r\n"
#define INSERT_MESSAGE "INS\r\n"
#define STYLE_CAHNGED_MESSAGE "STY\r\n"
#define DELETE_MESSAGE "DEL\r\n"
#define REMOVE_USER "RUS\n\r"
#define SENDING_FILE "FIL\n\r"
#define EDIT_ACCOUNT "EDT\n\r"
#define SHARE_CODE "SHR\n\r"
#define ADD_FILE "ADD\r\n"
#define REQUEST_USERNAME_LIST_MESSAGE "RUR\r\n"
#define USERNAME_LIST_FOR_FILE "LUR\r\n"
#define USERNAME_LIST_FOR_FILE "LUR\r\n"
#define FILE_INFORMATION_CHANGES "FIC\r\n"
#define DELETE_FILE "DFL\r\n"
#define ALIGNMENT_CHANGED_MESSAGE "ALN\r\n"


#define TIMEOUT 15000

enum SocketState{
    UNLOGGED,
    LOGGED,
    USER_MESSAGE_RECIVED,
    WAITING_LIST_OF_FILE,
    LIST_OF_FILE_RECIVED,
    WAITING_LIST_OF_ONLINE_USERS,
    WAITING_COMPLITE_FILE,
    WAITING_OK,
    EDIT_FILE_STATE
};

//enum alignment_type {
//    LEFT,
//    CENTER,
//    RIGHT,
//    JUSTIFY
//};

#endif // CONSTANTS_H
