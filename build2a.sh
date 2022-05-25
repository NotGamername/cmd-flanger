gcc -o flanger -DFIR_FILT main2.c flanger.c paUtils.c user_io.c \
	-Wall \
	-I/usr/local/include \
	-L/usr/local/lib -lsndfile -lportaudio
	