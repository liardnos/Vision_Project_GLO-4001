

all: SDL

configure:
	-rm -rf external
	mkdir external
	cd external && git clone https://github.com/libsdl-org/SDL
	cd external && git clone https://github.com/libsdl-org/SDL_image
	cd external && git clone https://github.com/libsdl-org/SDL_ttf
	make re -j6

re: clean reSDL all 


clean:
	rm -rf build
	rm -rf buildSDL
	mkdir build
	rm -rf $(NAME)


reSDL:
	rm -rf buildSDL
	mkdir buildSDL;
	cd buildSDL && cmake -D CMAKE_BUILD_TYPE=Release -D SDL_DISPLAY=ON .. && make -j6
	cp -r assets buildSDL/

SDL:
	clear
	cd buildSDL && cmake -D CMAKE_BUILD_TYPE=Release ..
	cd buildSDL && make -j 6 || clear && make

run: SDL
	cd buildSDL && ./ConfigurationSpace

valgrind: SDL
	##########################################################
	#                       VALGRIND                         #
	##########################################################
	cd buildSDL && valgrind --track-origins=yes --num-callers=500 ./ConfigurationSpace

# runServerClient: all
# 	terminator -x "./BoatFightServer; cat"
# 	terminator -x "./BoatFight; cat"

