/*
 * main.c
 * contains C code for Flappy GBA project
 */

#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 160

/* include the background image we are using */
#include "background.h"

/* include the sprite image we are using */
//#include "bird.h"

//#include "flappy.h"

//#include "sprites.h"

#include "upside_down.h"

/* include the tile map we are using */
#include "map.h"

/* Background for parallax effect */
#include "map2.h"

/* the tile mode flags needed for display control register */
#define MODE0 0x00
#define BG0_ENABLE 0x100
#define BG1_ENABLE 0x200 //enable second backgroung to be on

/* flags to set sprite handling in display control register */
#define SPRITE_MAP_2D 0x0
#define SPRITE_MAP_1D 0x40
#define SPRITE_ENABLE 0x1000


/* the control registers for the four tile layers */
volatile unsigned short* bg0_control = (volatile unsigned short*) 0x4000008;
volatile unsigned short* bg1_control = (volatile unsigned short*) 0x400000a;


/* palette is always 256 colors */
#define PALETTE_SIZE 256

/* there are 128 sprites on the GBA */
#define NUM_SPRITES 128

/* the display control pointer points to the gba graphics register */
volatile unsigned long* display_control = (volatile unsigned long*) 0x4000000;

/* the memory location which controls sprite attributes */
volatile unsigned short* sprite_attribute_memory = (volatile unsigned short*) 0x7000000;

/* the memory location which stores sprite image data */
volatile unsigned short* sprite_image_memory = (volatile unsigned short*) 0x6010000;

/* the address of the color palettes used for backgrounds and sprites */
volatile unsigned short* bg_palette = (volatile unsigned short*) 0x5000000;
volatile unsigned short* sprite_palette = (volatile unsigned short*) 0x5000200;

/* the button register holds the bits which indicate whether each button has
 * been pressed - this has got to be volatile as well
 */
volatile unsigned short* buttons = (volatile unsigned short*) 0x04000130;

/* scrolling registers for backgrounds */
volatile short* bg0_x_scroll = (unsigned short*) 0x4000010;
volatile short* bg0_y_scroll = (unsigned short*) 0x4000012;
volatile short* bg1_x_scroll = (unsigned short*) 0x4000014;



/* the bit positions indicate each button - the first bit is for A, second for
 * B, and so on, each constant below can be ANDED into the register to get the
 * status of any one button */
#define BUTTON_A (1 << 0)
#define BUTTON_B (1 << 1)
#define BUTTON_SELECT (1 << 2)
#define BUTTON_START (1 << 3)
#define BUTTON_RIGHT (1 << 4)
#define BUTTON_LEFT (1 << 5)
#define BUTTON_UP (1 << 6)
#define BUTTON_DOWN (1 << 7)
#define BUTTON_R (1 << 8)
#define BUTTON_L (1 << 9)

/* the scanline counter is a memory cell which is updated to indicate how
 * much of the screen has been drawn */
volatile unsigned short* scanline_counter = (volatile unsigned short*) 0x4000006;

/* wait for the screen to be fully drawn so we can do something during vblank */
void wait_vblank() {
    /* wait until all 160 lines have been updated */
    while (*scanline_counter < 160) { }
}

/* this function checks whether a particular button has been pressed */
unsigned char button_pressed(unsigned short button) {
    /* and the button register with the button constant we want */
    unsigned short pressed = *buttons & button;

    /* if this value is zero, then it's not pressed */
    if (pressed == 0) {
        return 1;
    } else {
        return 0;
    }
}

/* return a pointer to one of the 4 character blocks (0-3) */
volatile unsigned short* char_block(unsigned long block) {
    /* they are each 16K big */
    return (volatile unsigned short*) (0x6000000 + (block * 0x4000));
}

/* return a pointer to one of the 32 screen blocks (0-31) */
volatile unsigned short* screen_block(unsigned long block) {
    /* they are each 2K big */
    return (volatile unsigned short*) (0x6000000 + (block * 0x800));
}

/* flag for turning on DMA */
#define DMA_ENABLE 0x80000000

/* flags for the sizes to transfer, 16 or 32 bits */
#define DMA_16 0x00000000
#define DMA_32 0x04000000

/* pointer to the DMA source location */
volatile unsigned int* dma_source = (volatile unsigned int*) 0x40000D4;

/* pointer to the DMA destination location */
volatile unsigned int* dma_destination = (volatile unsigned int*) 0x40000D8;

/* pointer to the DMA count/control */
volatile unsigned int* dma_count = (volatile unsigned int*) 0x40000DC;

/* copy data using DMA */
void memcpy16_dma(unsigned short* dest, unsigned short* source, int amount) {
    *dma_source = (unsigned int) source;
    *dma_destination = (unsigned int) dest;
    *dma_count = amount | DMA_16 | DMA_ENABLE;
}

/* function to setup background 0 for this program */
void setup_background() {

    /* load the palette from the image into palette memory*/
    memcpy16_dma((unsigned short*) bg_palette, (unsigned short*) background_palette, PALETTE_SIZE);

    /* load the image into char block 0 */
    memcpy16_dma((unsigned short*) char_block(0), (unsigned short*) background_data,
            (background_width * background_height) / 2);

    /* set all control the bits in this register */
    //*bg0_control = 0 |    /* priority, 0 is highest, 3 is lowest */
        //(0 << 2)  |       /* the char block the image data is stored in */
        //(0 << 6)  |       /* the mosaic flag */
        //(1 << 7)  |       /* color mode, 0 is 16 colors, 1 is 256 colors */
        //(16 << 8) |       /* the screen block the tile data is stored in */
        //(1 << 13) |       /* wrapping flag */
        //(0 << 14);        /* bg size, 0 is 256x256 */

    /* Background setup for parallax */
    *bg1_control = 1 |
        (0 << 2) |
        (0 << 6) |
        (1 << 7) |
        (24 << 8) |
        (1 << 13) |
        (0 << 14);

    /* load the tile data into screen block 16 */
    //memcpy16_dma((unsigned short*) screen_block(16), (unsigned short*) map, map_width * map_height);

    /* Load map2 into screen block 24 */
    memcpy16_dma((unsigned short*) screen_block(24), (unsigned short*) map2, map2_width * map2_height);
}

/* just kill time */
void delay(unsigned int amount) {
    for (int i = 0; i < amount * 10; i++);
}

/* a sprite is a moveable image on the screen */
struct Sprite {
    unsigned short attribute0;
    unsigned short attribute1;
    unsigned short attribute2;
    unsigned short attribute3;
};

/* array of all the sprites available on the GBA */
struct Sprite sprites[NUM_SPRITES];
int next_sprite_index = 0;

/* the different sizes of sprites which are possible */
enum SpriteSize {
    SIZE_8_8,
    SIZE_16_16,
    SIZE_32_32,
    SIZE_64_64,
    SIZE_16_8,
    SIZE_32_8,
    SIZE_32_16,
    SIZE_64_32,
    SIZE_8_16,
    SIZE_8_32,
    SIZE_16_32,
    SIZE_32_64
};

/* function to initialize a sprite with its properties, and return a pointer */
struct Sprite* sprite_init(int x, int y, enum SpriteSize size,
        int horizontal_flip, int vertical_flip, int tile_index, int priority) {

    /* grab the next index */
    int index = next_sprite_index++;

    /* setup the bits used for each shape/size possible */
    int size_bits, shape_bits;
    switch (size) {
        case SIZE_8_8:   size_bits = 0; shape_bits = 0; break;
        case SIZE_16_16: size_bits = 1; shape_bits = 0; break;
        case SIZE_32_32: size_bits = 2; shape_bits = 0; break;
        case SIZE_64_64: size_bits = 3; shape_bits = 0; break;
        case SIZE_16_8:  size_bits = 0; shape_bits = 1; break;
        case SIZE_32_8:  size_bits = 1; shape_bits = 1; break;
        case SIZE_32_16: size_bits = 2; shape_bits = 1; break;
        case SIZE_64_32: size_bits = 3; shape_bits = 1; break;
        case SIZE_8_16:  size_bits = 0; shape_bits = 2; break;
        case SIZE_8_32:  size_bits = 1; shape_bits = 2; break;
        case SIZE_16_32: size_bits = 2; shape_bits = 2; break;
        case SIZE_32_64: size_bits = 3; shape_bits = 2; break;
    }

    int h = horizontal_flip ? 1 : 0;
    int v = vertical_flip ? 1 : 0;

    /* set up the first attribute */
    sprites[index].attribute0 = y |             /* y coordinate */
        (0 << 8) |          /* rendering mode */
        (0 << 10) |         /* gfx mode */
        (0 << 12) |         /* mosaic */
        (1 << 13) |         /* color mode, 0:16, 1:256 */
        (shape_bits << 14); /* shape */

    /* set up the second attribute */
    sprites[index].attribute1 = x |             /* x coordinate */
        (0 << 9) |          /* affine flag */
        (h << 12) |         /* horizontal flip flag */
        (v << 13) |         /* vertical flip flag */
        (size_bits << 14);  /* size */

    /* setup the second attribute */
    sprites[index].attribute2 = tile_index |   // tile index */
        (priority << 10) | // priority */
        (0 << 12);         // palette bank (only 16 color)*/

    /* return pointer to this sprite */
    return &sprites[index];
}

/* update all of the spries on the screen */
void sprite_update_all() {
    /* copy them all over */
    memcpy16_dma((unsigned short*) sprite_attribute_memory, (unsigned short*) sprites, NUM_SPRITES * 4);
}

/* setup all sprites */
void sprite_clear() {
    /* clear the index counter */
    next_sprite_index = 0;

    /* move all sprites offscreen to hide them */
    for(int i = 0; i < NUM_SPRITES; i++) {
        sprites[i].attribute0 = SCREEN_HEIGHT;
        sprites[i].attribute1 = SCREEN_WIDTH;
    }
}

/* set a sprite postion */
void sprite_position(struct Sprite* sprite, int x, int y) {
    /* clear out the y coordinate */
    sprite->attribute0 &= 0xff00;

    /* set the new y coordinate */
    sprite->attribute0 |= (y & 0xff);

    /* clear out the x coordinate */
    sprite->attribute1 &= 0xfe00;

    /* set the new x coordinate */
    sprite->attribute1 |= (x & 0x1ff);
}

/* move a sprite in a direction */
void sprite_move(struct Sprite* sprite, int dx, int dy) {
    /* get the current y coordinate */
    int y = sprite->attribute0 & 0xff;

    /* get the current x coordinate */
    int x = sprite->attribute1 & 0x1ff;

    /* move to the new location */
    sprite_position(sprite, x + dx, y + dy);
}

/* change the tile offset of a sprite */
void sprite_set_offset(struct Sprite* sprite, int offset) {
    /* clear the old offset */
    sprite->attribute2 &= 0xfc00;

    /* apply the new one */
    sprite->attribute2 |= (offset & 0x03ff);
}

/* setup the sprite image and palette */
void setup_sprite_image() {
    /* load the palette from the image into palette memory*/
    //memcpy16_dma((unsigned short*) sprite_palette, (unsigned short*) sprites_palette, PALETTE_SIZE);
    memcpy16_dma((unsigned short*) sprite_palette, (unsigned short*) upside_down_palette, PALETTE_SIZE);
    /* load the image into sprite image memory */
    //memcpy16_dma((unsigned short*) sprite_image_memory, (unsigned short*) sprites_data, (sprites_width * sprites_height) / 2);
    memcpy16_dma((unsigned short*) sprite_image_memory, (unsigned short*) upside_down_data, (upside_down_width * upside_down_height) / 2);
}

/* a struct for the bird's logic and behavior */
struct Bird {
    /* the actual sprite attribute info */
    struct Sprite* sprite;

    /* the x and y postion in pixels */
    int x, y;

    /* the bird's y velocity in 1/256 pixels/second */
    int yvel;

    /* the bird's y acceleration in 1/256 pixels/second^2 */
    int gravity; 

    /* the animation counter counts how many frames until we flip */
    int counter;

    /* whether the bird is moving right now or not */
    int move;

    /* Checks if there is a collision */
    int collision;

};

struct Pipe {
    struct Sprite* sprite;
    int x, y;
};

/* initialize the bird */
void bird_init(struct Bird* bird) {
    bird->x = 120;
    bird->y = 70;
    bird->yvel = 0;
    bird->gravity = 50;
    bird->move = 0;
    bird->counter = 0;
    bird->collision = 0;
    bird->sprite = sprite_init(bird->x, bird->y, SIZE_16_16, 0, 0, 0, 0);
}

/** Pipe initials **/


void pipe_init2(struct Pipe* pipe){
    pipe->x = 60;
    pipe->y = 128;
    pipe->sprite = sprite_init(pipe->x, pipe->y, SIZE_16_32, 0, 0, 12, 0);

}

void pipe_init3(struct Pipe* pipe){
    pipe->x = 60;
    pipe->y = 100;
    pipe->sprite = sprite_init(pipe->x, pipe->y, SIZE_16_32, 0,0,8,0);

}

void pipe_init4(struct Pipe* pipe){
    pipe->x = 100;
    pipe->y = 0;
    pipe->sprite = sprite_init(pipe->x, pipe->y, SIZE_16_32, 0,0,48,0);
}


void pipe_init5(struct Pipe* pipe){
    pipe->x = 100;
    pipe->y = 128;
    pipe->sprite = sprite_init(pipe->x, pipe->y, SIZE_16_32, 0,0,12,0);


}

void pipe_init6(struct Pipe* pipe){
    pipe->x = 100;
    pipe->y = 100;
    pipe->sprite = sprite_init(pipe->x, pipe->y, SIZE_16_32, 0,0,12,0);
}

void pipe_init7(struct Pipe* pipe){
    pipe->x = 100;
    pipe->y = 72;
    pipe->sprite = sprite_init(pipe->x, pipe->y, SIZE_16_32, 0,0,8,0);
}

void pipe_init8(struct Pipe* pipe){
    pipe->x = 60;
    pipe->y = 0;
    pipe->sprite = sprite_init(pipe->x, pipe->y, SIZE_16_32, 0,0,40,0);
}

void pipe_init9(struct Pipe* pipe){
    pipe->x = 60;
    pipe->y = 28;
    pipe->sprite = sprite_init(pipe->x, pipe->y, SIZE_16_32, 0,0,48,0);
}



int bird_right(struct Bird* bird) {
    // face right 
    if(bird->collision == 1){
        bird->move = 0;
        return 0;
    }
    bird->move = 1;

    //bird->x++;
    return 0;

}

int bird_down(struct Bird* bird){
    bird->move = 1;
    bird->y++;
    return 0;

}

int flappy_up(struct Bird* bird){
    bird->move = 1;
    bird->y--;
    return 0;

}

// stop the bird from flying left/right 

void bird_stop(struct Bird* bird) {
    bird->move = 0;
    //bird->frame = 0;
    bird->counter = 7;
    //sprite_set_offset(bird->sprite, bird->frame);
}

/* update the bird */
void bird_update(struct Bird* bird, int xscroll) {

    /* update animation if moving */
    if (bird->move) {
        bird->counter++;
    }

    /* set on screen position */
    sprite_position(bird->sprite, bird->x, bird->y);
}

void pipe_scroll(struct Pipe* pipe){
    pipe->x--;
    if(pipe->x < 0){
        pipe->x = 240;
    }
    sprite_position(pipe->sprite, pipe->x, pipe->y);

}

int collision(struct Bird* bird, struct Pipe* pipe){i
    //if the bird collides with a pipe (pipe->x - 16 is the left side of the pipe, pipe->y - 8 
    if(bird->x == pipe->x - 16 && bird->y >= pipe->y - 8){
        if(pipe->y == 0 || pipe->y == 28){ //if the pipe is upside down (aka y == 0)
            if(bird->y <= 50){
                bird->collision = 1;
                bird_stop(bird);
                return 1;
            }else {
                bird->collision = 0;
                return 0;
            }
        }
        bird->collision = 1;
        bird_stop(bird);
        return 1;
    }else {
        bird->collision = 0;
        return 0;
    }

}

int inc_collision(int collisions);

/* the main function */
int main() {
    /* we set the mode to mode 0 with bg0 on */
    *display_control = MODE0 | BG1_ENABLE | SPRITE_ENABLE | SPRITE_MAP_1D;

    /* setup the background 0 */
    setup_background();

    /* setup the sprite image data */
    setup_sprite_image();

    /* clear all the sprites on screen now */
    sprite_clear();

    /* create the bird */
    struct Bird bird;
    bird_init(&bird);

    /* create the pipe */

    struct Pipe pipe2;
    pipe_init2(&pipe2);

    struct Pipe pipe3;
    pipe_init3(&pipe3);

    struct Pipe pipe4;
    pipe_init4(&pipe4);

    struct Pipe pipe5;
    pipe_init5(&pipe5);

    struct Pipe pipe6;
    pipe_init6(&pipe6);

    struct Pipe pipe7;
    pipe_init7(&pipe7);

    struct Pipe pipe8;
    pipe_init8(&pipe8);

    struct Pipe pipe9;
    pipe_init9(&pipe9);

    /* set initial scroll to 0 */
    int xscroll = 0;

    int collisions = 0;

    /* loop forever */
    while (1) {
        /* update the bird */
        bird_update(&bird, xscroll);
        
        int num_of_collisions = inc_collision(collisions);
/*
        if(button_pressed(BUTTON_RIGHT)){
            if(collision(&bird, &pipe) || collision(&bird, &pipe2) || collision(&bird, &pipe3) || collision(&bird, &pipe4)){
                bird_stop(&bird);
                bird.x = 120;
                bird.y = 70;
            }else {
                xscroll++;
                pipe_scroll(&pipe);
                pipe_scroll(&pipe2);
                pipe_scroll(&pipe3);
                pipe_scroll(&pipe4);

            }
            //bird.collision = collision(&bird, &pipe);
            
            if(bird.collision == 1){
                bird_stop(&bird);
            }else {
                xscroll++;
                pipe_scroll(&pipe);
            }
            
        }   
*/
        /* now the arrow keys move the bird */
        if (button_pressed(BUTTON_UP)&&bird.y>0) {
            flappy_up(&bird);            

        } else if (button_pressed(BUTTON_DOWN)&&bird.y<144) {            
            bird_down(&bird);
           
        } /*else {
            bird_stop(&bird);
        }*/

        xscroll++;

        if(collision(&bird, &pipe2) || collision(&bird, &pipe3) || collision(&bird, &pipe4) || collision(&bird, &pipe5) || collision(&bird, &pipe6) || collision(&bird, &pipe7)
          || collision(&bird, &pipe8) || collision(&bird, &pipe9)){
            if(num_of_collisions == 5){
                bird_stop(&bird);
                bird.x = 120;
                bird.y = 70;
            }else {
                bird_stop(&bird);
                bird.x = bird.x - 10;
                collisions++;
            }
            //bird_stop(&bird);
            //bird.x = bird.x - 10;
            //bird.y = 70;
        }


        //pipe_scroll(&pipe);    
        pipe_scroll(&pipe2);
        pipe_scroll(&pipe3);
        pipe_scroll(&pipe4);
        pipe_scroll(&pipe5);
        pipe_scroll(&pipe6);
        pipe_scroll(&pipe7);
        pipe_scroll(&pipe8);
        pipe_scroll(&pipe9);
        /* wait for vblank before scrolling and moving sprites */
        wait_vblank();
        *bg0_x_scroll = xscroll;
        *bg1_x_scroll = xscroll * 1.5;
        sprite_update_all();

        /* delay some */
        delay(300);
    }
}

