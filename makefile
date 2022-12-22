

all: SDL

configure:
	-rm -rf external
	mkdir external
	cd external && git clone https://github.com/libsdl-org/SDL
	cd external && git clone https://github.com/libsdl-org/SDL_image
	make re

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
	cd buildSDL && ./Vision /home/loock/Kitti/2011_09_26/2011_09_26_drive_0001_sync/image_00/data

valgrind: SDL
	##########################################################
	#                       VALGRIND                         #
	##########################################################
	cd buildSDL && valgrind --track-origins=yes --num-callers=500 ./Vision /home/loock/Kitti/2011_09_26/2011_09_26_drive_0001_sync/image_00/data

# runServerClient: all
# 	terminator -x "./BoatFightServer; cat"
# 	terminator -x "./BoatFight; cat"

