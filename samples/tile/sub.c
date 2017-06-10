#include <road.h>
#include <stdint.h>

void main ()
{
	// Road should be lower priority.
	ROAD_Reset ();
	
	// Set the color gradient.
	const uint8_t grad_start = 87;
	uint8_t y;
	for (y=0; y<grad_start; y++)
		ROAD_PATTERN_0[y] = ROAD_SOLID_COLOR(0);
	for (y=grad_start; y<grad_start+127; y++)
		ROAD_PATTERN_0[y] = ROAD_SOLID_COLOR(y-grad_start);
	for (y=grad_start+127; y<224; y++)
		ROAD_PATTERN_0[y] = ROAD_SOLID_COLOR(127);
	
	// Swap buffers.
	ROAD_SwapBuffers ();

	// Infinite loop;
	for (;;);
}
