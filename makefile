CFLAGS = -Wall -I./SenderReceiver -pthread
LDFLAGS = -lrt

OBJ_A = A.o SenderReceiver/SenderReceiver.o
TARGET_A = A

OBJ_B = B.o SenderReceiver/SenderReceiver.o 
TARGET_B = B

all: $(TARGET_A) $(TARGET_B)

$(TARGET_A): $(OBJ_A)
	gcc $(CFLAGS) -o $(TARGET_A) $(OBJ_A) $(LDFLAGS)

$(TARGET_B): $(OBJ_B)
	gcc $(CFLAGS) -o $(TARGET_B) $(OBJ_B) $(LDFLAGS)

%.o: %.c
	gcc $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(TARGET_A) $(TARGET_B) $(OBJ_A) $(OBJ_B)
