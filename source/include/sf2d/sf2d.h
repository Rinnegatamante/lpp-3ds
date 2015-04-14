/**
 * @file sf2d.h
 * @author Sergi Granell (xerpi)
 * @date 22 March 2015
 * @brief sf2dlib header
 */

#ifndef SF2D_H
#define SF2D_H

#include <3ds.h>

#ifdef __cplusplus
extern "C" {
#endif

// Defines

/**
 * @brief Creates a new RGBA8 color
 * @param r the red component of the color to create
 * @param g the green component of the color to create
 * @param b the blue component of the color to create
 * @param a the alpha component of the color to create
 */
#define RGBA8(r, g, b, a) ((((r)&0xFF)<<24) | (((g)&0xFF)<<16) | (((b)&0xFF)<<8) | (((a)&0xFF)<<0))

// Enums

/**
 * @brief Data allocated on the RAM or VRAM
 */

typedef enum {
	SF2D_PLACE_RAM, /**< RAM allocated */
	SF2D_PLACE_VRAM /**< VRAM allocated */
} sf2d_place;

// Structs

/**
 * @brief Represents a two dimensional float vector
 */

typedef struct {
	float u, v;
} sf2d_vector_2f;

/**
 * @brief Represents a three dimensional float vector
 */

typedef struct {
	float x, y, z;
} sf2d_vector_3f;

/**
 * @brief Represents a four dimensional float vector
 */

typedef struct {
	float r, g, b, a;
} sf2d_vector_4f;

/**
 * @brief Represents a vertex containing position and color attributes
 */

typedef struct {
	sf2d_vector_3f position;
	sf2d_vector_4f color;
} sf2d_vertex_pos_col;

/**
 * @brief Represents a vertex containing position and texture coordinates
 */

typedef struct {
	sf2d_vector_3f position;
	sf2d_vector_2f texcoord;
} sf2d_vertex_pos_tex;

/**
 * @brief Represents a texture
 */

typedef struct {
	sf2d_place place;          /**< Where the texture data resides, RAM or VRAM */
	GPU_TEXCOLOR pixel_format; /**< Pixel format */
	int width;                 /**< Texture width */
	int height;                /**< Texture height */
	int pow2_w;                /**< Nearest power of 2 >= width */
	int pow2_h;                /**< Nearest power of 2 >= height */
	int data_size;             /**< Size of the raw texture data */
	void *data;                /**< Pointer to the data */
} sf2d_texture;

// Basic functions

/**
 * @brief Initializates the library
 * @return Whether the initialization has been successful or not
 */
int sf2d_init();

/**
 * @brief Finishes the library
 * @return Whether the finalization has been successful or not
 */
int sf2d_fini();

/**
 * @brief Starts a frame
 * @param screen target screen
 * @param side target eye (ony for top screen)
 */
void sf2d_start_frame(gfxScreen_t screen, gfx3dSide_t side);

/**
 * @brief Ends a frame, should be called on pair with sf2d_start_frame
 */
void sf2d_end_frame();

/**
 * @brief Swaps the framebuffers, should be called once after all the frames has been finished
 */
void sf2d_swapbuffers();

/**
 * @brief Enables or disables the VBlank waiting
 * @param enable whether to enable or disable the VBlank waiting
 */
void sf2d_set_vblank_wait(int enable);

/**
 * @brief Returns the FPS (frames per second)
 * @return the current FPS
 */
float sf2d_get_fps();

/**
 * @brief Allocates memory from a temporary pool. The pool will be emptied after a sf2d_swapbuffers call
 * @param size the number of bytes to allocate
 */
void *sf2d_pool_malloc(u32 size);

/**
 * @brief Allocates aligned memory from a temporary pool. Works as sf2d_pool_malloc
 * @param size the number of bytes to allocate
 * @param alignment the alignment to where allocate the memory
 */
void *sf2d_pool_memalign(u32 size, u32 alignment);

/**
 * @brief Empties the temporary pool
 */
void sf2d_pool_reset();

/**
 * @brief Sets the screen clear color
 * @param color the color
 */
void sf2d_set_clear_color(u32 color);

// Draw functions
/**
 * @brief Draws a line
 * @param x0 x coordinate of the first dot
 * @param y0 y coordinate of the first dot
 * @param x1 x coordinate of the second dot
 * @param y1 y coordinate of the sceond dot
 * @param color the color to draw the line
 */
void sf2d_draw_line(int x0, int y0, int x1, int y1, u32 color);

/**
 * @brief Draws a rectangle
 * @param x x coordinate of the top left corner of the rectangle
 * @param y y coordinate of the top left corner of the rectangle
 * @param w rectangle width
 * @param w rectangle height
 * @param color the color to draw the rectangle
 */
void sf2d_draw_rectangle(int x, int y, int w, int h, u32 color);

/**
 * @brief Draws a rotated rectangle
 * @param x x coordinate of the top left corner of the rectangle
 * @param y y coordinate of the top left corner of the rectangle
 * @param w rectangle width
 * @param w rectangle height
 * @param color the color to draw the rectangle
 * @param rad rotation (in radians) to draw the rectangle
 */
void sf2d_draw_rectangle_rotate(int x, int y, int w, int h, u32 color, float rad);

// Texture

/**
 * @brief Creates an empty texture.
 * The returned texture has the data allocated,
 * this means that the raw pixel data can be filled
 * just after the return.
 * @param width the width of the texture
 * @param height the height of the texture
 * @param pixel_format the pixel_format of the texture
 * @param place where to allocate the texture
 * @return a pointer to the newly created texture
 */
sf2d_texture *sf2d_create_texture(int width, int height, GPU_TEXCOLOR pixel_format, sf2d_place place);

/**
 * @brief Frees a texture
 * @param texture pointer to the texture to freeze
 */
void sf2d_free_texture(sf2d_texture *texture);

/**
 * @brief Fills an already allocated texture from a RGBA8 source
 * @param dst pointer to the destination texture to fill
 * @param rgba8 pointer to the RGBA8 data to fill from
 * @param source_w width (in pixels) of the RGAB8 source
 * @param source_h height (in pixels) of the RGAB8 source
 */
void sf2d_fill_texture_from_RGBA8(sf2d_texture *dst, const void *rgba8, int source_w, int source_h);

/**
 * @brief Binds a texture to a GPU texture unit
 * @param texture the texture to bind
 * @param unit GPU texture unit to bind to
 */
void sf2d_bind_texture(const sf2d_texture *texture, GPU_TEXUNIT unit);

/**
 * @brief Draws a texture
 * @param texture the texture to draw
 * @param x the x coordinate to draw the texture to
 * @param y the y coordinate to draw the texture to
 */
void sf2d_draw_texture(const sf2d_texture *texture, int x, int y);

/**
 * @brief Draws a texture with rotation
 * @param texture the texture to draw
 * @param x the x coordinate to draw the texture to
 * @param y the y coordinate to draw the texture to
 * @param rad rotation (in radians) to draw the texture
 */
void sf2d_draw_texture_rotate(const sf2d_texture *texture, int x, int y, float rad);

/**
 * @brief Draws a part of a texture
 * @param texture the texture to draw
 * @param x the x coordinate to draw the texture to
 * @param y the y coordinate to draw the texture to
 * @param tex_x the starting point (x coordinate) where to start drawing
 * @param tex_y the starting point (y coordinate) where to start drawing
 * @param tex_w the width to draw from the starting point
 * @param tex_h the height to draw from the starting point
 */
void sf2d_draw_texture_part(const sf2d_texture *texture, int x, int y, int tex_x, int tex_y, int tex_w, int tex_h);

/**
 * @brief Draws a texture with scaling
 * @param texture the texture to draw
 * @param x the x coordinate to draw the texture to
 * @param y the y coordinate to draw the texture to
 * @param x_scale the x scale
 * @param y_scale the y scale
 */
void sf2d_draw_texture_scale(const sf2d_texture *texture, int x, int y, float x_scale, float y_scale);

/**
 * @brief Draws a part of a texture, with rotation and scaling
 * @param texture the texture to draw
 * @param x the x coordinate to draw the texture to
 * @param y the y coordinate to draw the texture to
 * @param rad rotation (in radians) to draw the texture
 * @param tex_x the starting point (x coordinate) where to start drawing
 * @param tex_y the starting point (y coordinate) where to start drawing
 * @param tex_w the width to draw from the starting point
 * @param tex_h the height to draw from the starting point
 * @param x_scale the x scale
 * @param y_scale the y scale
 */
void sf2d_draw_texture_rotate_cut_scale(const sf2d_texture *texture, int x, int y, float rad, int tex_x, int tex_y, int tex_w, int tex_h, float x_scale, float y_scale);

/**
 * @brief Tiles a texture
 * @param texture the texture to tile
 * @param x the x coordinate to draw the texture to
 * @param y the y coordinate to draw the texture to
 */
void sf2d_texture_tile32(sf2d_texture *texture);

#ifdef __cplusplus
}
#endif

#endif
