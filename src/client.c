/* Chat client
 *
 * Copyright (C) 2018 Sauli Hirvi
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <arpa/inet.h>
#include <curses.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "chat.h"
#include "chat_message.h"
#include "client.h"
#include "error_log.h"
#include "session.h"

static volatile int keep_running = 1;

static WINDOW *mainwindow;
static WINDOW *inputwindow;

void intHandler(__attribute__((unused)) int dummy) {
    wprintw(mainwindow, "interrupt!");
    keep_running = 0;
}

int printtime() {
    time_t rawtime;
    struct tm *timeinfo;

    time(&rawtime);
    timeinfo = localtime(&rawtime);
    wprintw(mainwindow, "[%02d:%02d:%02d] ", timeinfo->tm_hour,
            timeinfo->tm_min, timeinfo->tm_sec);

    return 0;
}

int handshake(int socket, chatSession *session) {
    char *response;
    char *part;

    response = calloc(1, MAX_MSG);

    /* send greeting */
    send(socket, "AHOY", MAX_MSG, 0);
    printtime();
    wprintw(mainwindow, "Greeting sent\n");

    /* receive response */
    recv(socket, response, MAX_MSG, 0);

    /* get first part of response */
    part = strtok(response, ":");

    if (strcmp(part, "AHOY-HOY") == 0) {
        printtime();
        wprintw(mainwindow, "Correct response received\n");
        /* Response correct, get token */
        part = strtok(NULL, ":");
        char *token = calloc(1, strlen(part));
        strcpy(token, part);
        session->token = token;
        wprintw(mainwindow, "Authentication token: %s\n", session->token);
    } else {
        return -1;
    }
    return 0;
}

/*
 * Prompt for nickname and store it into the session.
 */
int set_nickname(chatSession *session) {
    char *nick;
    unsigned int i;

    nick = calloc(MAX_NICK, sizeof(char));
    wprintw(inputwindow, "Select a nickname:\n");
    wscanw(inputwindow, " %99[^\n]", nick);

    /* Replace colons with underscore in nickname */
    for (i = 0; i < strlen(nick); i++) {
        if (nick[i] == ':') {
            nick[i] = '_';
        }
    }

    session->nickname = nick;
    return 0;
}

int init_socket() {
    struct sockaddr_in server_address;
    int network_socket;
    int status;

    /* Create a socket */
    network_socket = socket(AF_INET, SOCK_STREAM, 0);

    /* Specify an address for the socket */
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(8002);
    server_address.sin_addr.s_addr = inet_addr("127.0.0.1");

    /* Connect to the server */
    status = connect(network_socket, (struct sockaddr *)&server_address,
                     sizeof(server_address));

    /* Check for connection error */
    if (status == -1) {
        wprintw(mainwindow,
                "There was an error connecting to the remote socket.\n\n");
        endwin();
        exit(1);
    }
    return network_socket;
}

/*
 * Create and return a new curses window
 */
WINDOW *create_newwin(int height, int width, int starty, int startx) {
    WINDOW *local_win;
    local_win = newwin(height, width, starty, startx);
    wrefresh(local_win);
    return local_win;
}

int main() {
    int network_socket;
    int status;
    char *server_response;
    char *input;
    ssize_t len;
    chatSession *session;

    char *input_buffer;
    int input_pos = 0;

    /* Add interrupt handler to catch CTRL-C */
    signal(SIGINT, intHandler);

    initscr();

    mainwindow = create_newwin(LINES - 3, COLS, 0, 0);
    inputwindow = create_newwin(3, COLS, LINES - 3, 0);

    wprintw(mainwindow, "Main window initialized.\n");
    wprintw(inputwindow, "Input window initialized.\n");

    network_socket = init_socket();

    /* Initialize the session variable */
    session = create_session();

    printtime();
    wprintw(mainwindow, "Connected to the server\n");

    server_response = calloc(MAX_MSG, sizeof(char));
    input = calloc(MAX_MSG, sizeof(char));
    input_buffer = calloc(MAX_MSG, sizeof(char));

    /* Shake hands with the server */
    status = handshake(network_socket, session);
    if (status == -1) {
        wprintw(mainwindow, "Handshake failed.\n");
        endwin();
        exit(1);
    }

    /* Select nick for user */
    set_nickname(session);

    /* Set curses options */
    nodelay(inputwindow, TRUE);
    noecho();
    scrollok(mainwindow, TRUE);
    scrollok(inputwindow, TRUE);

    wclear(inputwindow);
    wprintw(inputwindow, "CHAT >> ");

    /* Main loop */
    while (keep_running) {
        wrefresh(mainwindow);
        wrefresh(inputwindow);

        int c = wgetch(inputwindow);
        if (c == 13 || c == 10) { /* Newline */
            /* Handle submit */
            chatMessage *message = malloc(sizeof(chatMessage));
            message->token = session->token;
            message->nickname = session->nickname;
            message->message = input_buffer;

            char *msg_str = format_message(message);

            /* Send message to server */
            len = send(network_socket, msg_str, MAX_MSG, 0);

            memset(input_buffer, 0, MAX_MSG);
            input_pos = 0;
            wclear(inputwindow);
            wprintw(inputwindow, "CHAT >> ");

        } else if (c >= 32) {
            wprintw(inputwindow, "%c", c);
            input_buffer[input_pos++] = c;
        }

        /* Get response from server */
        len = recv(network_socket, server_response, MAX_MSG, MSG_DONTWAIT);

        if (len < 0) {
            usleep(10000);
            continue;
        }

        chatMessage *msg = parse_message(server_response);

        /* Print out the response */
        printtime();
        wprintw(mainwindow, "<%s> %s\n", msg->nickname, msg->message);

        /* Clear the arrays */
        memset(server_response, 0, MAX_MSG);
        memset(input, 0, MAX_MSG);
    }

    endwin();

    free(input);
    /* Close the socket */
    close(network_socket);

    return 0;
}
