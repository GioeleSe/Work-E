Mario's code will present 3 main code modules:
1. UDP server -> incoming messages handler (commands from server)
2. UDP client -> sending feedback, debug and hearthbeat
3. HW interface -> interface for the actual robot (on 2 layers, the upper one for logical instructions and the lower one for physical ones)