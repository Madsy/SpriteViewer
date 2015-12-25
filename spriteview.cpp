#include <fileobserver.hpp>
#include <unistd.h>
#include <getopt.h>
#include <SDL.h>
#include <SDL_image.h>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <string>
#include <memory>
//#include <regex>
#include <sstream>
#include <utility>

//example usage:
//spriteview -w 16 -h 16 -s 2 -f 16 -i 0:16 -t sprtmap.png

struct options {
	int imageWidth;
	int imageHeight;
	int spriteWidth;
	int spriteHeight;
	int scale;
	int tiled;
	int fps;
	int frameStartIndex;
	int frameEndIndex;
	std::string imgPath;
};

options viewopts = {
	-1,	//width of spritemap
	-1,	//height of spritemap
	-1,	//width of sprite/tile
	-1,	//height of sprite/tile
	 1, //scale
	 0, //should not tile
	 8, //8 frames per second
	 0, //first sprite index
	 1, //last sprite index + 1.
	"" //no default path
};

std::string appname;
std::vector<SDL_Surface*> spriteList;
SDL_Surface* spriteMap;
SDL_Surface* screen;
int spritesPerRow = -1;
int spritesPerCol = -1;
bool fullscreen = false;
bool fullscreenSupported = true;

int viewportWidth_Windowed = 640;
int viewportHeight_Windowed = 360;

//fullscreen resolution is detected in init()
int viewportWidth_Fullscreen = 0;
int viewportHeight_Fullscreen = 0;

int viewportWidth = viewportWidth_Windowed;
int viewportHeight = viewportHeight_Windowed;

static void print_usage(){
	printf("Usage: %s [options] file\n", appname.c_str());
	printf("Options:\n");
	printf("  %-20s%s\n", "-w <width>", "Sprite width");
	printf("  %-20s%s\n", "-h <height>", "Sprite height");
	printf("  %-20s%s\n", "-s <scale>", "Sprite scale");
	printf("  %-20s%s\n", "-f <fps>", "Animation frames per second");
	printf("  %-20s%s\n", "-i start:end", "The start- and end sprite indices.");
	printf("  %-20s%s\n", "-t", "Enable tiling/repeat");
}

static void init(){
	if(SDL_Init(SDL_INIT_VIDEO) < 0){
		printf("%s - error: Failed to initialize the SDL library\n", appname.c_str());
		exit(1);
	}
	int imgFlags = (IMG_INIT_JPG | IMG_INIT_PNG | IMG_INIT_TIF);
	if((IMG_Init(imgFlags) & imgFlags) != imgFlags){
		printf("%s - error: Failed to initialize the SDL_Image library\n", appname.c_str());
		SDL_Quit();
		exit(1);
	}

	SDL_Surface* tmpSpriteMap = IMG_Load(viewopts.imgPath.c_str());
	if(!tmpSpriteMap){
		printf("%s - error: Failed to open \"%s\"\n", appname.c_str(), viewopts.imgPath.c_str());
		IMG_Quit();
		SDL_Quit();
		exit(1);
	}

	viewopts.imageWidth = tmpSpriteMap->w;
	viewopts.imageHeight = tmpSpriteMap->h;
	
	if((viewopts.imageWidth % viewopts.spriteWidth) != 0){
		printf("%s - error: Sprite map width is not a multiple of the sprite width\n", appname.c_str());
		IMG_Quit();
		SDL_Quit();
		exit(1);
	}

	if((viewopts.imageHeight % viewopts.spriteHeight) != 0){
		printf("%s - error: Sprite map height is not a multiple of the sprite height\n", appname.c_str());
		IMG_Quit();
		SDL_Quit();
		exit(1);
	}

	spritesPerRow = viewopts.imageWidth / viewopts.spriteWidth;
	spritesPerCol = viewopts.imageHeight / viewopts.spriteHeight;

	if(!spritesPerRow || !spritesPerCol){
		printf("%s - error: Sprite map size is smaller than the sprite size\n", appname.c_str());
		IMG_Quit();
		SDL_Quit();
		exit(1);
	}

	int total_num_sprites = spritesPerRow*spritesPerCol;

	if((viewopts.frameStartIndex < -1) ||
		(viewopts.frameStartIndex >= total_num_sprites) ||
		(viewopts.frameEndIndex < -1) ||
		(viewopts.frameEndIndex >= total_num_sprites))
	{
		printf("%s - error: Sprite index out of range\n", appname.c_str());
		IMG_Quit();
		SDL_Quit();
		exit(1);
	}

	if(viewopts.frameStartIndex > viewopts.frameEndIndex){
		printf("%s - error: Start sprite index is greater than end index\n", appname.c_str());
		IMG_Quit();
		SDL_Quit();
		exit(1);
	}


	//Find the best resolution for fullscreen
	SDL_Rect** videoMode = SDL_ListModes(0, SDL_HWSURFACE | SDL_DOUBLEBUF | SDL_FULLSCREEN);

	if(!videoMode){
		//no fullscreen support
		fullscreenSupported = false;
	} else if(videoMode == (SDL_Rect**)-1){
		//no restrictions, set to 1080p
		viewportWidth_Fullscreen = 1920;
		viewportHeight_Fullscreen = 1080;
	} else {
		int highestResFound = 0;
		for(int i=0; videoMode[i] != 0; i++){
			int numPixelsInMode = videoMode[i]->w * videoMode[i]->h; 
			if(highestResFound < numPixelsInMode){
				viewportWidth_Fullscreen = videoMode[i]->w;
				viewportHeight_Fullscreen = videoMode[i]->h;
				highestResFound = numPixelsInMode;			  
			} 
		}
		if(!viewportWidth_Fullscreen || !viewportHeight_Fullscreen){
			fullscreenSupported = false;
		}
	}

	//default window size is 640x360
	screen = SDL_SetVideoMode(viewportWidth, viewportHeight, 32, SDL_HWSURFACE | SDL_RESIZABLE | SDL_DOUBLEBUF);
	if(!screen){
		printf("%s - error: Failed to set video mode\n", appname.c_str());
		IMG_Quit();
		SDL_Quit();
		exit(1);
	}
	//clipping
	SDL_SetClipRect(screen, 0);
	
	//allocate spritemap with same pixel format as the window
	spriteMap = SDL_DisplayFormatAlpha(tmpSpriteMap);
	if(!spriteMap){
		printf("%s - error: Out of memory while converting video surface\n", appname.c_str());
		IMG_Quit();
		SDL_Quit();
		exit(1);
	}
	SDL_FreeSurface(tmpSpriteMap);
}

static SDL_Surface* create_sprite_surface(int width, int height, SDL_Surface* ref){
	SDL_Surface* s;
	s = SDL_CreateRGBSurface(
			SDL_HWSURFACE,
			width,
			height,
			ref->format->BitsPerPixel,
			ref->format->Rmask,
			ref->format->Gmask,
			ref->format->Bmask,
			ref->format->Amask
	);
	if(!s) return 0;
	SDL_SetAlpha(s, 0, 0);
	return s;
}

static void init_sprites(){

	int x,y,iy,ix;
	int ret;

	if(SDL_MUSTLOCK(spriteMap)){
		SDL_LockSurface(spriteMap);
	}

	const unsigned int* src = (unsigned int*)spriteMap->pixels;
	int srcPitch = spriteMap->pitch / sizeof(unsigned int);

	for(y = 0; y < spritesPerCol; y++){
		for(x = 0; x < spritesPerRow; x++){
			SDL_Surface* sprite = create_sprite_surface(
				viewopts.spriteWidth * viewopts.scale,
				viewopts.spriteHeight * viewopts.scale,
				spriteMap
			);
			if(!sprite){				
				printf("%s - error: Out of memory while creating sprites.\n", appname.c_str());
				IMG_Quit();
				SDL_Quit();
				exit(1);
			}
			
			if(SDL_MUSTLOCK(sprite)){
				SDL_LockSurface(sprite);
			}

			unsigned int* dst = (unsigned int*)sprite->pixels;
			int dstPitch = sprite->pitch / sizeof(unsigned int);

			for(iy=0; iy < viewopts.spriteHeight; iy++){
				for(ix=0; ix < viewopts.spriteWidth; ix++){
					int dstY = iy * viewopts.scale;
					int dstX = ix * viewopts.scale;
					int srcY = (y * viewopts.spriteHeight) + iy;
					int srcX = (x * viewopts.spriteWidth) + ix;
					unsigned int color = src[srcX + srcY * srcPitch];
					for(int sy = 0; sy < viewopts.scale; sy++){
						for(int sx = 0; sx < viewopts.scale; sx++){
							dst[(dstX + sx) + (dstY + sy) * dstPitch] = color;
						}
					}
				}
			}

			if(SDL_MUSTLOCK(sprite)){
				SDL_UnlockSurface(sprite);
			}
			
			spriteList.push_back(sprite);
		}
	}
	
	if(SDL_MUSTLOCK(spriteMap)){
		SDL_UnlockSurface(spriteMap);
	}
}

static void destroy_sprites(){
	size_t i;
	for(i = 0; i < spriteList.size(); i++){
		SDL_FreeSurface(spriteList[i]);		
	}
	spriteList.clear();
}

class file_observer_impl : public file_observer {
public:
	file_observer_impl(const std::string& name) : file_observer(name){}
	~file_observer_impl(){}
protected:
	void on_write(){
		//printf("Spritemap modified outside of viewer. Reloading.");
		SDL_Surface* tmpSpriteMap = IMG_Load(viewopts.imgPath.c_str());
		if(!tmpSpriteMap){
			return;
		}
		if(tmpSpriteMap->w != spriteMap->w){
			return;
		}
		if(tmpSpriteMap->h != spriteMap->h){
			return;
		}
		SDL_FreeSurface(spriteMap);
		spriteMap = SDL_DisplayFormatAlpha(tmpSpriteMap);
		SDL_FreeSurface(tmpSpriteMap);
		destroy_sprites();
		init_sprites();
	}
};

static void draw_sprite(int spriteIdx){
	int scale = viewopts.scale;

	SDL_Surface* sprite = spriteList[spriteIdx];

	int srcWidth = sprite->w;
	int srcHeight = sprite->h;
	int srcPitch = sprite->pitch / sizeof(unsigned int);

	int dstWidth = screen->w;
	int dstHeight = screen->h;
	int dstPitch = screen->pitch / sizeof(unsigned int);

	SDL_Rect dstRect;
	const unsigned int* src = (unsigned int*)sprite->pixels;
	unsigned int* dst = (unsigned int*)screen->pixels;

	if(viewopts.tiled &&
	   (srcWidth < dstWidth) &&
	   (srcHeight < dstHeight)){
		int x,y;
		for(y = 0; y < dstHeight; y+=srcHeight){
			for(x = 0; x < dstWidth; x+=srcWidth){
				dstRect.y = y;
				dstRect.x = x;
				dstRect.w = 0;
				dstRect.h = 0;
				SDL_BlitSurface(sprite, 0, screen, &dstRect);
			}
		}
		SDL_Flip(screen);
	} else {
		//render a single tile centered on screen
		
		dstRect.y = (dstHeight - srcHeight) / 2;
		dstRect.x = (dstWidth - srcWidth) / 2; 
		dstRect.w = 0;
		dstRect.h = 0;

		SDL_BlitSurface(sprite, 0, screen, &dstRect);
		SDL_Flip(screen);
	}
}

void main_loop(){
	SDL_Event event;
	bool running = true;
	Uint32 lastTime = SDL_GetTicks();
	Uint32 timeAccum = 0;
	Uint32 msPerFrame = 1000/viewopts.fps;
	Uint32 frameOffs = 0;
	Uint32 numFrames = viewopts.frameEndIndex - viewopts.frameStartIndex + 1;

	std::shared_ptr<file_observer> file_change_event(new file_observer_impl(viewopts.imgPath));	

	while(running){
		while(SDL_PollEvent(&event)){
			switch(event.type){
			case SDL_VIDEORESIZE:
				{					
					viewportWidth = event.resize.w;
					viewportHeight = event.resize.h;
					if(viewportWidth < 256){
						viewportWidth = 256;
					}
					if(viewportHeight < 256){
						viewportHeight = 256;
					}
					screen = SDL_SetVideoMode(viewportWidth, viewportHeight, 32, SDL_HWSURFACE | SDL_RESIZABLE | SDL_DOUBLEBUF);
					if(!screen){
						printf("%s - error: Window resize failed\n", appname.c_str());
						IMG_Quit();
						SDL_Quit();
						exit(1);						
					}
				}
				break;
			case SDL_KEYDOWN:
				//running = false;
				switch(event.key.keysym.sym){
				case SDLK_ESCAPE:
					running = false;
					break;
				case 'f':
					{
						if(!fullscreenSupported){
							break;
						}
						if(!fullscreen){
							//entering fullscreen
							viewportWidth = viewportWidth_Fullscreen;
							viewportHeight = viewportHeight_Fullscreen;
							screen = SDL_SetVideoMode(viewportWidth, viewportHeight, 32, SDL_HWSURFACE | SDL_DOUBLEBUF | SDL_FULLSCREEN);
							if(!screen){
								printf("%s - error: Window resize failed\n", appname.c_str());
								viewportWidth = viewportWidth_Windowed;
								viewportHeight = viewportHeight_Windowed;
								screen = SDL_SetVideoMode(viewportWidth, viewportHeight, 32, SDL_HWSURFACE | SDL_RESIZABLE | SDL_DOUBLEBUF);
							} else {
								fullscreen = true;
							}
						} else {
							//leaving fullscreen
							viewportWidth = viewportWidth_Windowed;
							viewportHeight = viewportHeight_Windowed;
							screen = SDL_SetVideoMode(viewportWidth, viewportHeight, 32, SDL_HWSURFACE | SDL_RESIZABLE | SDL_DOUBLEBUF);
							if(!screen){
								printf("%s - error: Window resize failed\n", appname.c_str());
								IMG_Quit();
								SDL_Quit();
								exit(1);						
							} else {
								fullscreen = false;
							}
						}
					}
				}
				break;
			case SDL_QUIT:
				running = false;
				break;  
			}
		}
		//check if file has been modified externally
		file_change_event->poll();

		Uint32 currentTime = SDL_GetTicks();
		Uint32 timeDelta = currentTime - lastTime;
		lastTime = currentTime;
		timeAccum += timeDelta;
		if(timeAccum > 1000)
			timeAccum = 0;

		while(timeAccum >= msPerFrame){
			frameOffs = (frameOffs + 1) % numFrames;
			int frameIndex = viewopts.frameStartIndex + frameOffs;
			draw_sprite(frameIndex);
			timeAccum -= msPerFrame;
		}
	}
}

int main(int argc, char* argv[]){
        if(argc < 2){
		print_usage();
		exit(1);
	}

	appname = std::string(argv[0]);

	char c;
	std::string i_param;

	while ((c = getopt (argc, argv, "tw:h:s:f:i:")) != -1){
		switch(c){
			case 't':
				viewopts.tiled = 1;
			break;
			case 'w':
				viewopts.spriteWidth = atoi(optarg);
			break;
			case 'h':
				viewopts.spriteHeight = atoi(optarg);
			break;
			case 's':
				viewopts.scale = atoi(optarg);
			break;
			case 'f':
				viewopts.fps = atoi(optarg);
			break;
			case 'i':
				i_param = std::string(optarg);				
			break;
			default:
				print_usage();
				exit(1);
			break;
		}		
	}

	//Parse -i argument. It has the syntax "-i 16:8", that is, -i start:end
	if(i_param.empty()){
		printf("%s - error: missing sprite start and end index\n", argv[0]);
		exit(1);
	} else {
		std::istringstream i_strm;
		char i_delim;
		i_strm.str(i_param);
		if(!(i_strm >> viewopts.frameStartIndex)){
			printf("%s - error: \"-i %s\" incorrect syntax\n", argv[0], i_param.c_str());
			exit(1);
		}
		if(!(i_strm >> i_delim) || i_delim != ':'){
			printf("%s - error: \"-i %s\" incorrect syntax\n", argv[0], i_param.c_str());
			exit(1);
		}
		if(!(i_strm >> viewopts.frameEndIndex)){
			printf("%s - error: \"-i %s\" incorrect syntax\n", argv[0], i_param.c_str());
			exit(1);
		}
	}

	int i = optind;

	if(i >= argc){
		printf("%s: No input given.\n", argv[0]);
		exit(1);
	}
	viewopts.imgPath = std::string(argv[i]);

	if(viewopts.spriteWidth < 4){
		printf("%s - error: missing sprite width or invalid value (min=4px)\n", argv[0]);
		exit(1);
	}
	if(viewopts.spriteHeight < 4){
		printf("%s - error: missing sprite height or invalid value (min=4px)\n", argv[0]);
		exit(1);
	}

	printf("Running with these parameters:\n");
	printf("  %-20s %d\n",		"sprite map width:",	viewopts.imageWidth);
	printf("  %-20s %d\n",		"sprite map height:",	viewopts.imageHeight);
	printf("  %-20s %d\n",		"sprite width:",		viewopts.spriteWidth);
	printf("  %-20s %d\n",		"sprite height:",		viewopts.spriteHeight);
	printf("  %-20s %d\n",		"scale:", 				viewopts.scale);
	printf("  %-20s %s\n",		"tiled/repeat mode",	(viewopts.tiled ? "enabled" : "disabled"));
	printf("  %-20s %d fps\n",	"animation speed:", 	viewopts.fps);
	printf("  %-20s %d\n",		"first frame index:",	viewopts.frameStartIndex);
	printf("  %-20s %d\n",		"last frame index:",	viewopts.frameEndIndex);
	printf("  %-20s %s\n",		"image path:",			viewopts.imgPath.c_str());

	init();
	init_sprites();
	main_loop();
	destroy_sprites();
	IMG_Quit();
	SDL_Quit();
	return 0;
}

