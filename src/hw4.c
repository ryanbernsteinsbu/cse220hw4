#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT1 2201
#define PORT2 2202
#define SO_REUSEPORT 15
#define BUFFER_SIZE 1024

struct Ship *player_1_ships[5];
struct Ship *player_2_ships[5];
char **player_1_shots;
char **player_2_shots;
int last_flag = 0;
int width;
int height;

int shapes[7][4][6] = {
    // Shape 1 (Yellow)
    {
        {1, 0, 1, 1, 0, 1}, // Rotation 1
        {1, 0, 1, 1, 0, 1}, // Rotation 2
        {1, 0, 1, 1, 0, 1}, // Rotation 3
        {1, 0, 1, 1, 0, 1}  // Rotation 4
    },
    // Shape 2 (Blue)
    {
        {0, 1, 0, 2, 0, 3}, // Rotation 1
        {1, 0, 2, 0, 3, 0}, // Rotation 2
        {0, 1, 0, 2, 0, 3}, // Rotation 3
        {1, 0, 2, 0, 3, 0}  // Rotation 4
    },
    // Shape 3 (Red)
    {
        {1, 0, 1, -1, 2, -1}, // Rotation 1
        {0, 1, 1, 1, 1, 2}, // Rotation 2
        {1, 0, 1, -1, 2, -1}, // Rotation 3
        {0, 1, 1, 1, 1, 2}  // Rotation 4
    },
    // Shape 4 (Orange)
    {
        {0, 1, 0, 2, 1, 2},  // Rotation 1
        {0, 1, 1, 0, 2, 0}, // Rotation 2
        {1, 0, 1, 1, 1, 2}, // Rotation 3
        {1, 0, 2, 0, 2, -1}  // Rotation 4
    },
    // Shape 5 (Green)
    {
        {1, 0, 1, 1, 2, 1}, // Rotation 1
        {0, 1, 1, 0, 1, -1}, // Rotation 2
        {1, 0, 1, 1, 2, 1}, // Rotation 2
        {0, 1, 1, 0, 1, -1}  // Rotation 4
    },
    // Shape 6 (Pink)
    {
        {1, 0, 1, -1, 1, -2}, // Rotation 1
        {0, 1, 1, 1, 2, 1},  // Rotation 2
        {1, 0, 0, 1, 0, 2},   // Rotation 3
        {1, 0, 2, 0, 2, 1}   // Rotation 4
    },
    // Shape 7 (Purple)
    {
        {1, 0, 1, 1, 2, 0},  // Rotation 1
        {1, 0, 1, 1, 1, -1},  // Rotation 2
        {1, 0, 1, -1, 2, 0}, // Rotation 3
        {0, 1, 0, 2, 1, 1}  // Rotation 4
    }
};

typedef struct Point
{
    int x;
    int y;
    int hit; //bool
}Point;

typedef struct Ship{
    struct Point origin;
    struct Point point1;
    struct Point point2;
    struct Point point3;
}Ship;

int ship_left(int player);

struct Ship *generate_ship(int type, int rotation, int col, int row){
    struct Ship *out = (struct Ship *)malloc(sizeof(struct Ship));
    out->origin.x = col;
    out->origin.y = row;
    out->point1.x = col + shapes[type - 1][rotation - 1][0];
    out->point1.y = row + shapes[type - 1][rotation - 1][1];
    out->point2.x = col + shapes[type - 1][rotation - 1][2];
    out->point2.y = row + shapes[type - 1][rotation - 1][3];
    out->point3.x = col + shapes[type - 1][rotation - 1][4];
    out->point3.y = row + shapes[type - 1][rotation - 1][5];
    out->origin.hit = 0;
    out->point1.hit = 0;
    out->point2.hit = 0;
    out->point3.hit = 0;
    return out;
}

void init_board(int _width, int _height){
    player_1_shots = malloc(sizeof(char*) * _width);
    player_2_shots = malloc(sizeof(char*) * _width);
    for (int i = 0; i < _width; i++) {
        player_1_shots[i] = malloc(sizeof(char) * _height);
        player_2_shots[i] = malloc(sizeof(char) * _height);
    }
    for(int i = 0; i < _width; i++){
        for(int j = 0; j <  _height; j++){
            player_1_shots[i][j] = '0';
            player_2_shots[i][j] = '0';
        }
    }

    width = _width;
    height = _height;
}

int num_tokens(char* input){
    int token_count = 0;
    int str_count = 0;
    for(int i = 0; input[i] != '\0'; i++){
        str_count++;
    }
    char temp_input[str_count];
    strcpy(temp_input, input);

    char *token = strtok(temp_input, " ");
    while (token != NULL) {
        token_count++;
        token = strtok(NULL, " ");
    }

    return token_count;
}

int is_integer(const char *str) {
    char *endptr;
    strtol(str, &endptr, 10);
    return *endptr == '\0';
}

char *check_ships(struct Ship **player_ships){
    struct Point checklist[20];
    for(int i = 0; i < 5; i++){
        checklist[(i * 4)] = player_ships[i]->origin;
        checklist[(i * 4) + 1] = player_ships[i]->point1;
        checklist[(i * 4) + 2] = player_ships[i]->point2;
        checklist[(i * 4) + 3] = player_ships[i]->point3;
    }

    for(int i = 0; i < 20; i++){
        struct Point test = checklist[i];
        // printf("%d, %d\n", test.x, test.y);
        if(test.x < 0 || test.x >= width || test.x < 0 || test.y >= height){
            return "E 302";
        }
        for(int j = i + 1; j < 20; j++){
            struct Point check = checklist[j];
            if(test.x == check.x && test.y == check.y){
                return "E 303";
            }
        }
    }
    return "A";
}

char *init_ships(int player, char* packet){ //“I 1 1 0 0 1 1 0 2 1 1 0 4 1 1 2 2 1 1 2 0”
    if(packet[0] == 'F'){
        return "H 0";
        last_flag = 0;
    }
    if(num_tokens(packet) != 21){
        return "E 201";
    }
    int numbers[20];
    int count = 0;

    int str_count = 0;
    for(int i = 0; packet[i] != '\0'; i++){
        str_count++;
    }
    char temp_input[str_count];
    strcpy(temp_input, packet);

    char *token = strtok(temp_input, " "); // Split by space
    if (strcmp(token, "I") != 0) { //check first token is I
        return "E 101";
    }

    while (token != NULL) {
        if (strcmp(token, "I") != 0) { // Skip I
            if(!is_integer(token)){
                return "E 101";
            }
            numbers[count++] = atoi(token); // Convert to integer and store
        }
        token = strtok(NULL, " ");//iterate
    }
    
    struct Ship **player_ships = (player == 1) ? player_1_ships : player_2_ships; 

    for(int i = 0, j = 0; i < 20; i += 4, j++){
        if(numbers[i] > 4 || numbers[i] < 1){
            return "E 300";
        }
        if(numbers[i+1] > 4 || numbers[i+1] < 1){
            return "E 301";
        }
        player_ships[j] = generate_ship(numbers[i],numbers[i+1], numbers[i+2], numbers[i+3]);
    }
    return check_ships((player == 1) ? player_1_ships : player_2_ships);
}

char *shoot(char* packet, int player){
    if(num_tokens(packet) != 3){
        return "E 102";
    }

    int numbers[2];
    int count = 0;

    int str_count = 0;
    for(int i = 0; packet[i] != '\0'; i++){
        str_count++;
    }
    char temp_input[str_count];
    strcpy(temp_input, packet);

    char *token = strtok(temp_input, " "); // Split by space
    if (strcmp(token, "S") != 0) { //check first token is S
        return "E 102";
    }

    while (token != NULL) {
        if (strcmp(token, "S") != 0) { // Skip S
            if(!is_integer(token)){
                return "E 102";
            }
            numbers[count++] = atoi(token); // Convert to integer and store
        }
        token = strtok(NULL, " ");//iterate
    }

    if(numbers[0] < 0 || numbers[0] >= width || numbers[1] < 0 || numbers[1] >= height){ //out of range
            return "E 400";
    }


    char **shot_list = (player == 1) ? player_1_shots : player_2_shots; //check if weve shot that spot alr
    
    if(shot_list[numbers[0]][numbers[1]] != '0'){
        return "E 401";
    }

    struct Ship **ship_list = (player == 1) ? player_2_ships : player_1_ships;

    char* out = malloc(sizeof(char) * 6);

    for(int i = 0; i < 5; i++){
        struct Ship* check = ship_list[i];
        if(numbers[0] == check->origin.x && numbers[1] == check->origin.y){
            shot_list[numbers[0]][numbers[1]] = 'X';
            check->origin.hit = 1;
            sprintf(out, "R %d H", ship_left(player));
            return out;
        }
        if(numbers[0] == check->point1.x && numbers[1] == check->point1.y){
            shot_list[numbers[0]][numbers[1]] = 'X';
            check->point1.hit = 1;
            sprintf(out, "R %d H", ship_left(player));
            return out;
        }
        if(numbers[0] == check->point2.x && numbers[1] == check->point2.y){
            shot_list[numbers[0]][numbers[1]] = 'X';
            check->point2.hit = 1;
            sprintf(out, "R %d H", ship_left(player));
            return out;
        }
        if(numbers[0] == check->point3.x && numbers[1] == check->point3.y){
            shot_list[numbers[0]][numbers[1]] = 'X';
            check->point2.hit = 1;
            sprintf(out, "R %d H", ship_left(player));
            return out;
        }
    }
    shot_list[numbers[0]][numbers[1]] = 'O';
    sprintf(out, "R %d M", ship_left(player));
    return out;
}

int ship_left(int player){
    int ships_left = 0;
    struct Ship **ship_list = (player == 1) ? player_2_ships : player_1_ships;
    for(int i = 0; i < 5; i++){
        struct Ship *shipoi = ship_list[i]; 
        if(shipoi->origin.hit && shipoi->point1.hit && shipoi->point2.hit && shipoi->point3.hit){
            continue;
        }
        ships_left++;
    }
    return ships_left;
}

char *query(int player){
    //char *buffer = malloc(sizeof(char) * 1024);//should be big enough
    char *out = (char*)malloc(sizeof(char) * 1024);
    int ships_left = ship_left(player);
    struct Ship **ship_list = (player == 1) ? player_2_ships : player_1_ships;

    int index = 0;
    char header[100] = {0};
    sprintf(header, "G %d", ships_left);
    sprintf(out, header);

    char **points = (player == 1) ? player_1_shots : player_2_shots;

    for(int i = 0; i < width; i++){
        for(int j = 0; j < height; j++){
            char poi = points[i][j];
            if(poi == '0'){
                continue;
            }
            char buffer[100] = {0};
            sprintf(buffer, " %c %d %d", (poi == 'X') ? 'H' : 'M', i, j);
            strcat(out, buffer);
        }
        
    }
    //strcat(out, '\0');
    return out;
}

char *input1 (char* packet, int player){
    // printf("%s\n", packet);
    if(packet[0] != 'B' && packet[0] != 'F'){
        // printf("yello");
        return "E 100";
    }
    if(packet[0] == 'F'){
        last_flag = 0;
        return "H 0";
    }
    if(player == 1){
        if(num_tokens(packet) != 3){
            return "E 200";
        }

        int numbers[2];
        int count = 0;

        int str_count = 0;
        for(int i = 0; packet[i] != '\0'; i++){
            str_count++;
        }
        char temp_input[str_count];
        strcpy(temp_input, packet);

        char *token = strtok(temp_input, " "); // Split by space

        while (token != NULL) {
            if (strcmp(token, "B") != 0) { // Skip B
                if(!is_integer(token)){
                    return "E 200";
                }
                numbers[count++] = atoi(token); // Convert to integer and store
            }
        token = strtok(NULL, " ");//iterate
        }

        if(numbers[0] < 10 || numbers[1] < 10){ //out of range
            return "E 200";
        }

        init_board(numbers[0],numbers[1]);
        return "A";
    }else{
        if(num_tokens(packet) != 1){
            return "E 200";
        }
        return "A";
    }

}

char *input2(char *packet, int player){
    if(packet[0] != 'F' && packet[0] != 'S' & packet[0] != 'Q'){
        // printf("yello");
        return "E 102";
    }
    char* out = malloc(sizeof(char) * 1024);
    char *tmp;
    switch(packet[0]){
        case 'F':
            strcpy(out, "H 0"); //need to implement forfeit
            last_flag = 0;
            return out;
        case 'S':
            tmp = shoot(packet, player);
            if(ship_left((player == 1) ? 2 : 1) == 0){
                last_flag++;
            }
            sprintf(out, tmp);
            //free(tmp);
            return out;
        case 'Q':
            tmp = query(player);
            sprintf(out, tmp);
            free(tmp);
            return out;

        default:
            printf("something is very wrong");
            break;
    }

}

int main() {

    int listen_fd1, listen_fd2, conn_fd1, conn_fd2;
    struct sockaddr_in address1, address2;
    int opt = 1;
    int addrlen1 = sizeof(address1);
    int addrlen2 = sizeof(address2);
    char buffer1[BUFFER_SIZE] = {0};
    char buffer2[BUFFER_SIZE] = {0};
    char output_buffer[BUFFER_SIZE] = {0};
    int ended = 0;

    // Create sockets
    if ((listen_fd1 = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    if ((listen_fd2 = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Set socket options
    if (setsockopt(listen_fd1, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(listen_fd2, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))");
        exit(EXIT_FAILURE);
    }



    // Bind sockets to ports
    address1.sin_family = AF_INET;
    address1.sin_addr.s_addr = INADDR_ANY;
    address1.sin_port = htons(PORT1);

    address2.sin_family = AF_INET;
    address2.sin_addr.s_addr = INADDR_ANY;
    address2.sin_port = htons(PORT2);


    if (bind(listen_fd1, (struct sockaddr *)&address1, sizeof(address1)) < 0) {
        perror("[Server] bind() failed.");
        exit(EXIT_FAILURE);
    }

    if (bind(listen_fd2, (struct sockaddr *)&address2, sizeof(address2)) < 0) {
        perror("[Server] bind() failed.");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(listen_fd1, 3) < 0) {
        perror("[Server] listen() failed.");
        exit(EXIT_FAILURE);
    }
    // Listen for incoming connections
    if (listen(listen_fd2, 3) < 0) {
        perror("[Server] listen() failed.");
        exit(EXIT_FAILURE);
    }

    printf("[Server] Running on port %d and port %d\n", PORT1, PORT2);

    // Accept incoming connection for port 1
    if ((conn_fd1 = accept(listen_fd1, (struct sockaddr *)&address1, (socklen_t *)&addrlen1)) < 0) {
        perror("[Server] accept() failed.");
        exit(EXIT_FAILURE);
    }

    // Accept incoming connection for port 2
    if ((conn_fd2 = accept(listen_fd2, (struct sockaddr *)&address2, (socklen_t *)&addrlen2)) < 0) {
        perror("[Server] accept() failed.");
        exit(EXIT_FAILURE);
    }
    memset(output_buffer, 0, BUFFER_SIZE);
    while (1) { //begin loop
        do{
            // printf("Ran Player 1 LOOP");
            memset(output_buffer, 0, BUFFER_SIZE);
            memset(buffer1, 0, BUFFER_SIZE);
            int nbytes1 = read(conn_fd1, buffer1, BUFFER_SIZE);
            if (nbytes1 <= 0) {
                perror("[Server] read() failed.");
                exit(EXIT_FAILURE);
            }
            printf("[Server] Received from client: %s\n", buffer1);
            printf(output_buffer);
            sprintf(output_buffer,"%s",input1(buffer1, 1));
            // strcat(input1(buffer1, 1), output_buffer);
            send(conn_fd1,output_buffer,strlen(output_buffer),0);

        } while (output_buffer[0] == 'E');

        
        do{
            if((output_buffer[0] == 'H'|| last_flag == 1) && ended == 0){
                if(output_buffer[0] == 'H'){
                    ended++;
                }
                send(conn_fd2,"H 1",strlen("H 1"),0);
                break;
            }
            memset(output_buffer, 0, BUFFER_SIZE);
            memset(buffer2, 0, BUFFER_SIZE);
            int nbytes2 = read(conn_fd2, buffer2, BUFFER_SIZE);
            if (nbytes2 <= 0) {
                perror("[Server] read() failed.");
                exit(EXIT_FAILURE);
            }
            printf("[Server] Received from client: %s\n", buffer2);
            sprintf(output_buffer,"%s",input1(buffer2, 2));
            send(conn_fd2,output_buffer,strlen(output_buffer),0);
        } while (output_buffer[0] == 'E');
        break;
    }

    do{ // init loop 1
        if((output_buffer[0] == 'H' || last_flag == 1)
         && ended == 0){
            if(output_buffer[0] == 'H'){
                ended++;
            }
            char out[3];
            out[0] = 'H';
            out[1] = ' ';
            out[2] = (last_flag == 1) ? '0' : '1';
            send(conn_fd2,out ,3,0);
            break;
        }
        memset(output_buffer, 0, BUFFER_SIZE);
        memset(buffer1, 0, BUFFER_SIZE);
        int nbytes1 = read(conn_fd1, buffer1, BUFFER_SIZE);
        if (nbytes1 <= 0) {
            perror("[Server] read() failed.");
            exit(EXIT_FAILURE);
        }
        printf("[Server] Received from client: %s\n", buffer1);
        printf(output_buffer);
        char *tmp = init_ships(1, buffer1);
        sprintf(output_buffer,"%s", tmp);
        send(conn_fd1,output_buffer,strlen(output_buffer),0);
        printf("%d\n", output_buffer[0] != 'E');
    } while (output_buffer[0] == 'E');

    do{ // init loop 2
        if((output_buffer[0] == 'H' || last_flag == 1)&& ended == 0){
            if(output_buffer[0] == 'H'){
                ended++;
            }
            char out[3];
            out[0] = 'H';
            out[1] = ' ';
            out[2] = (last_flag == 0) ? '1' : '0';
            send(conn_fd2,out ,3,0);
            break;
        }
        memset(output_buffer, 0, BUFFER_SIZE);
        memset(buffer2, 0, BUFFER_SIZE);
        int nbytes2 = read(conn_fd2, buffer2, BUFFER_SIZE);
        if (nbytes2 <= 0) {
            perror("[Server] read() failed.");
            exit(EXIT_FAILURE);
        }
        printf("[Server] Received from client: %s\n", buffer2);
        printf(output_buffer);
        sprintf(output_buffer,"%s",init_ships(2, buffer2));
        send(conn_fd2,output_buffer,strlen(output_buffer),0);
        printf("%d\n", output_buffer[0] != 'E');
    } while (output_buffer[0] == 'E');

    while (1) { //main loop
        do{
            if((output_buffer[0] == 'H' || last_flag == 1) && ended == 0){
                if(output_buffer[0] == 'H'){
                    ended++;
                }
                char out[3];
                out[0] = 'H';
                out[1] = ' ';
                out[2] = (last_flag == 0) ? '1' : '0';
                send(conn_fd2,out ,3,0);
                break;
            }
            memset(output_buffer, 0, BUFFER_SIZE);
            memset(buffer1, 0, BUFFER_SIZE);
            int nbytes1 = read(conn_fd1, buffer1, BUFFER_SIZE);
            if (nbytes1 <= 0) {
                perror("[Server] read() failed.");
                exit(EXIT_FAILURE);
            }
            printf("[Server] Received from client: %s\n", buffer1);
            // printf(output_buffer);
            char *tmp = input2(buffer1, 1);
            sprintf(output_buffer,"%s",tmp);
            free(tmp);
            // strcat(input1(buffer1, 1), output_buffer);
            send(conn_fd1,output_buffer,strlen(output_buffer),0);
            printf("%d\n", output_buffer[0] != 'E');
        } while (output_buffer[0] == 'E');
        
        do{
            if((output_buffer[0] == 'H' || last_flag == 1)&& ended == 0){
                if(output_buffer[0] == 'H'){
                    ended++;
                }
                char out[3];
                out[0] = 'H';
                out[1] = ' ';
                out[2] = (last_flag == 0) ? '1' : '0';
                printf(out + '\n');
                send(conn_fd2,out ,3,0);
                break;
            }
            memset(output_buffer, 0, BUFFER_SIZE);
            memset(buffer2, 0, BUFFER_SIZE);
            int nbytes2 = read(conn_fd2, buffer2, BUFFER_SIZE);
            if (nbytes2 <= 0) {
                perror("[Server] read() failed.");
                exit(EXIT_FAILURE);
            }
            printf("[Server] Received from client: %s\n", buffer2);
            char *tmp = input2(buffer2, 2);
            sprintf(output_buffer,"%s",tmp);
            free(tmp);
            send(conn_fd2,output_buffer,strlen(output_buffer),0);
        } while (output_buffer[0] == 'E' || output_buffer[0] == 'G');
    }
}

