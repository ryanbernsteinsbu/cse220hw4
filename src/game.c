#include <stdio.h>
#include <stdlib.h>
#include <string.h>


// Format: shapes[shape][rotation][tile_offset (col, row)]






int main (){
    init_board(10,10);
    init_ships(1, "I 1 1 0 0 1 1 0 2 1 1 0 4 1 1 2 2 1 1 2 0");
    shoot("S 0 0",2);
    printf("%s",query(2));
}