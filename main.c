#include <SDL2/SDL.h>
#include <SDL2/SDL_timer.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Quote of the day: "Sometimes you need to write bad code" - Me

#define CHARACTER_WIDTH 4
#define WINDOW_WIDTH

typedef struct  Vec2d{
    int x, y;
} Vec2d;

void render_cursor(SDL_Renderer *renderer, Vec2d cursor) {
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_Rect cursor_rect = {cursor.x * (CHARACTER_WIDTH + 1), cursor.y * (CHARACTER_WIDTH + 1), CHARACTER_WIDTH, CHARACTER_WIDTH};
    SDL_RenderDrawRect(renderer, &cursor_rect);
}

// Just resizes the display texture to fit the window
void render_display(SDL_Renderer *renderer, SDL_Window *window, SDL_Texture *display){
    SDL_SetRenderTarget(renderer, NULL);

    int display_width, display_height, access;
    Uint32 format;
    SDL_QueryTexture(display, &format, &access, &display_width, &display_height);

    int window_width, window_height;
    SDL_GetWindowSize(window, &window_width, &window_height);

    double scale_x = (double) window_width / (double) display_width;
    double scale_y = (double) window_height / (double) display_height;
    double min_scale = SDL_min(scale_x, scale_y);

    int to_center_x = (window_width - display_width * min_scale) / 2;
    int to_center_y = (window_height - display_height * min_scale) / 2;

    SDL_Rect rect = {to_center_x, to_center_y, display_width * min_scale, display_height * min_scale};

    SDL_RenderCopy(renderer, display, NULL, &rect);
}

int find_character_index(char character, char *characters) {
    for (int i=0;i<strlen(characters);i++) {
        if (character == characters[i]) return i;
    }
    return -1;
}

// Iteratively renders each character in a line
void render_text(SDL_Renderer *renderer, char *text, char *characters, SDL_Texture *font, Vec2d position) {
    for (int i=0;i<strlen(text);i++) {
        char character = text[i];
        if (character == 0) continue;

        int index = find_character_index(character, characters);
        SDL_Rect char_texture_position = { index * (CHARACTER_WIDTH + 1), 0, CHARACTER_WIDTH, CHARACTER_WIDTH };
        SDL_Rect char_canvas_position = { position.x + i * (CHARACTER_WIDTH + 1), position.y, CHARACTER_WIDTH, CHARACTER_WIDTH };

        if (index == -1) {
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            SDL_RenderDrawRect(renderer, &char_canvas_position);
            continue;
        }

        SDL_RenderCopy(renderer, font, &char_texture_position, &char_canvas_position);
    }
}

void clear_screen(SDL_Renderer *renderer, SDL_Texture *display) {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

    SDL_SetRenderTarget(renderer, NULL);
    SDL_RenderClear(renderer);

    SDL_SetRenderTarget(renderer, display);
    SDL_RenderClear(renderer);
}

// Resizes all lines in the text buffer
void resize_line_length(char **text, int *line_length, int lines) {
    *line_length *= 2;
    for (int i=0;i<lines;i++) {
        text[i] = realloc(text[i], *line_length);
        memset(&text[i][*line_length/2], 0, *line_length/2);
    }
}

int main(int argc, char **argv) {
    // SDL setup (not really important)
    SDL_Init(SDL_INIT_EVERYTHING);
    SDL_Window *window = SDL_CreateWindow("Editor", 0, 0, 500, 160, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, 0);
    int display_width = 40, display_height = 32;
    SDL_Texture *display = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, display_width * (CHARACTER_WIDTH + 1), display_height * (CHARACTER_WIDTH + 1));

    // Spritesheet used to write characters
    SDL_Surface *temp_font = SDL_LoadBMP("smol_font.bmp");
    SDL_Texture *font = SDL_CreateTextureFromSurface(renderer, temp_font);
    SDL_FreeSurface(temp_font);

    // Character list used to index character images from spritesheet
    char characters[] = "abcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*()`~-_=+\\|[]}{';:/?.>,<\" ";
    Vec2d cursor = {0, 0};
    // Used for moving screen around
    Vec2d scroll = {0, 0};
    // lines from the end of the screen before it starts scrolling
    int start_scrolling_at = 12;

    // Just ints to check whether the user wants to search or replace a character
    int search_char = 0, replace_char = 0;
    int done = 0;
    while (!done) {
        clear_screen(renderer, display);

        // Calculates when the cursor is moving past a certain limit and moving the screen approriately
        scroll.x += (cursor.x - scroll.x > display_width - start_scrolling_at) -((cursor.x - scroll.x < start_scrolling_at) && cursor.x >= start_scrolling_at);
        scroll.y += (cursor.y - scroll.y > display_height - start_scrolling_at) -((cursor.y - scroll.y < start_scrolling_at) && cursor.y >= start_scrolling_at);

        // Event loop
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            // Check if the program was closed and saves file
            if (event.type == SDL_QUIT) {
                done = 1;
            }

            if (event.type == SDL_KEYDOWN) {
                // Delete char before cursor
                if (event.key.keysym.sym == SDLK_BACKSPACE) {
                    // Move text to the left if the cursor is not at the beginning of the line
                    if (cursor.x > 0) {
                        memmove(&text[cursor.y][cursor.x - 1], &text[cursor.y][cursor.x], strlen(&text[cursor.y][cursor.x]) + 1);
                        cursor.x--;
                    // Copy text from line to the line up if the cursor is at the beginning of the line
                    } else if (cursor.y > 0) {
                        if (text[cursor.y][cursor.x] > 0) {
                            if (strlen(text[cursor.y]) > line_length - strlen(text[cursor.y - 1])) {
                                resize_line_length(text, &line_length, lines);
                            }
                            memmove(&text[cursor.y - 1][strlen(text[cursor.y - 1])], text[cursor.y], strlen(text[cursor.y]));
                        }

                        for (int i=0;i<lines - cursor.y - 1;i++) {
                            memmove(text[cursor.y + i], text[cursor.y + i + 1], line_length);
                        }

                        free(text[lines - 1]);
                        text = realloc(text, (lines - 1) * sizeof(char*));
                        lines--;
                        cursor.y--;
                        cursor.x = strlen(&text[cursor.y][cursor.x]);
                    }
                // Move cursor around
                } else if (event.key.keysym.sym == SDLK_LEFT && cursor.x > 0) {
                    cursor.x--;
                } else if (event.key.keysym.sym == SDLK_RIGHT && text[cursor.y][cursor.x] > 0) {
                    cursor.x++;
                } else if (event.key.keysym.sym == SDLK_UP && cursor.y > 0) {
                    cursor.y--;
                    while (text[cursor.y][cursor.x - 1] == 0 && cursor.x != 0) cursor.x--;
                } else if (event.key.keysym.sym == SDLK_DOWN && cursor.y < lines - 1) {
                    cursor.y++;
                    while (text[cursor.y][cursor.x - 1] == 0 && cursor.x != 0) cursor.x--;
                // Make new line
                } else if (event.key.keysym.sym == SDLK_RETURN) {
                    text = realloc(text, (1 + lines) * sizeof(char*));
                    text[lines] = calloc(line_length + 1, sizeof(char));

                    if (cursor.y < lines - 1) {
                        for (int i=lines - cursor.y - 2;i>=0;i--) {
                            memmove(text[cursor.y + i + 2], text[cursor.y + i + 1], line_length);
                        }
                    }

                    memset(text[cursor.y + 1], 0, line_length);

                    if (cursor.x <= strlen(text[cursor.y])) {
                        memmove(&text[cursor.y + 1][0], &text[cursor.y][cursor.x], strlen(&text[cursor.y][cursor.x]) + 1);
                        memset(&text[cursor.y][cursor.x], 0, strlen(&text[cursor.y][cursor.x]));
                    }

                    lines++;
                    cursor.y++;
                    cursor.x = 0;
                // Save file
                } else if ((event.key.keysym.mod & KMOD_LCTRL) && (event.key.keysym.sym == SDLK_s)) {
                    FILE *fptr = fopen(argv[1], "w+");

                    for (int i=0;i<lines;i++) {
                        fprintf(fptr, "%s\n", text[i]);
                    }

                    fclose(fptr);
                // Toggle replace char state
                } else if ((event.key.keysym.mod & KMOD_LCTRL) && (event.key.keysym.sym == SDLK_r)) {
                    replace_char = 1;
                // Toggle search char state
                } else if ((event.key.keysym.mod & KMOD_LCTRL) && (event.key.keysym.sym == SDLK_f)) {
                    search_char = 1;
                }
            }

            if (event.type == SDL_TEXTINPUT) {
                char *input = event.text.text;
                // Search for character
                if (search_char) {
                    for (int i=cursor.x;i<strlen(text[cursor.y]) - 1;i++) {
                        if (text[cursor.y][i + 1] == input[0]) {
                            cursor.x = i + 1;
                        }
                    }
                    search_char = 0;
                    continue;
                // Replace char
                } else if (replace_char) {
                    if (text[cursor.y][cursor.x] > 0) {
                        text[cursor.y][cursor.x] = input[0];
                    }
                    replace_char = 0;
                    continue;
                } else {
                // Text input
                for (int i=0;i<strlen(input);i++) {
                    if (find_character_index(tolower(input[i]), characters) == -1) continue;
                    if (strlen(text[cursor.y]) > line_length - 2) {
                        resize_line_length(text, &line_length, lines);
                    }
                    memmove(&text[cursor.y][cursor.x + 1], &text[cursor.y][cursor.x], strlen(&text[cursor.y][cursor.x]) + 1);
                    text[cursor.y][cursor.x] = tolower(input[i]);
                    cursor.x++;
                }
                }
            }
        }

        SDL_SetRenderTarget(renderer, display);

        for (int i=0;i<lines;i++) {
            render_text(renderer, text[i], characters, font, (Vec2d) { .x = -scroll.x * (CHARACTER_WIDTH + 1), .y = i * (CHARACTER_WIDTH + 1) - scroll.y * (CHARACTER_WIDTH + 1)});
        }

        render_cursor(renderer, (Vec2d) {.x = cursor.x - scroll.x, .y = cursor.y - scroll.y});
        render_display(renderer, window, display);
        SDL_RenderPresent(renderer);
    }

    // Intentionally wrote Selection sort so that you can actually see the characters move, and because I was lazy lol
    // It didn't specify in the instructions that the sorting algorithm had to be useful in any way))))
    // This sorts all the characters in each line as an outro
    for (int i=0;i<lines;i++) {
        for (int j=0;j<(int)strlen(text[i]) - 1;j++) {
            int current_minimum = j;
            for (int k=j+1;k<strlen(&text[i][j]);k++) {
                if (text[i][k] < text[i][current_minimum]) {
                    current_minimum = k;
                }
            }
            char temp = text[i][j];
            text[i][j] = text[i][current_minimum];
            text[i][current_minimum] = temp;
            clear_screen(renderer, display);
            for (int i=0;i<lines;i++) {
                render_text(renderer, text[i], characters, font, (Vec2d) { .x = 0, .y = i * (CHARACTER_WIDTH + 1)});
            }
            SDL_Delay(5);
            render_display(renderer, window, display);
            SDL_RenderPresent(renderer);
        }
    }

    // Just free up everything
    for (int i=0;i<lines;i++) {
        free(text[i]);
    }
    free(text);
    SDL_DestroyTexture(display);
    SDL_DestroyTexture(font);
    SDL_DestroyWindow(window);
    SDL_DestroyRenderer(renderer);
    SDL_Quit();
}
