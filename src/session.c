/* Chat session structure and related functionality.
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

#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "chat.h"
#include "session.h"

/*
 * Generate an access token for the client
 */
char *create_token(char *token, size_t size) {
    static char charset[] = "abcdefghijklmnopqrstuvwxyz0123456789";
    char *random_string = NULL;
    size_t n;

    random_string = calloc((size + 1), sizeof(char));

    if (random_string) {
        srand(time(NULL));
        for (n = 0; n < size; n++) {
            int key = rand() % (int)(sizeof(charset) - 1);
            random_string[n] = charset[key];
        }

        random_string[size] = '\0';
    }

    token = random_string;
    return random_string;
}

/*
 * Replace old nickname with a new one
 */
void session_replace_nick(chatSession *session, char *newnick) {
    memset(session->nickname, 0, MAX_MSG);
    strcpy(session->nickname, newnick);
    /*
        for (i = 0; i < MAX_NICK; i++) {
            session->nickname[i] = '\0';
        }
        for (i = 6; i < (int)strlen(input_buffer); i++) {
            session->nickname[i - 6] = input_buffer[i];
        }
    */
}

/*
 * Create stub session object.
 */
chatSession *create_session(void) {
    chatSession *session = calloc(1, sizeof(chatSession));
    session->token = "";
    session->nickname = "";
    return session;
}
