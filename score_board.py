#!/usr/bin/env python
# Display a runtext with double-buffering.
from samplebase import SampleBase
from rgbmatrix import graphics, RGBMatrix, RGBMatrixOptions
import paho.mqtt.client as mqtt
import time
import random
import logging
import os
FIRST_RECONNECT_DELAY = 1
RECONNECT_RATE = 2
MAX_RECONNECT_COUNT = 12
MAX_RECONNECT_DELAY = 60



class RunText(SampleBase):
    def __init__(self, *args, **kwargs):
        super(RunText, self).__init__(*args, **kwargs)
        self.parser.add_argument("-t", "--text", help="The text to scroll on the RGB LED panel", default="")
        self.hitCups1 = [0,0,0,0,0,0]
        self.hitCups2 = [0,0,0,0,0,0]
        self.team1score_int = 0
        self.team2score_int = 0
        self.team1score = str(self.team1score_int)
        self.team2score = str(self.team2score_int)
        self.team1Topic = "team/1/score"
        self.team2Topic = "team/2/score"
        self.allCupsHit1 = False
        self.allCupsHit2 = False


    def on_connect1(self, client, userdata, flags, rc):
        if rc == 0 and client.is_connected():
            print("Connected to MQTT Broker!")
            client.subscribe(self.team1Topic)
        else:
            print(f'Failed to connect, return code {rc}')

    def on_connect2(self, client, userdata, flags, rc):
        if rc == 0 and client.is_connected():
            print("Connected to MQTT Broker!")
            client.subscribe(self.team2Topic)
        else:
            print(f'Failed to connect, return code {rc}')


    def on_disconnect(self,client, userdata, rc):
        logging.info("Disconnected with result code: %s", rc)
        reconnect_count, reconnect_delay = 0, FIRST_RECONNECT_DELAY
        while reconnect_count < MAX_RECONNECT_COUNT:
            logging.info("Reconnecting in %d seconds...", reconnect_delay)
            time.sleep(reconnect_delay)

            try:
                client.reconnect()
                logging.info("Reconnected successfully!")
                return
            except Exception as err:
                logging.error("%s. Reconnect failed. Retrying...", err)

            reconnect_delay *= RECONNECT_RATE
            reconnect_delay = min(reconnect_delay, MAX_RECONNECT_DELAY)
            reconnect_count += 1
        logging.info("Reconnect failed after %s attempts. Exiting...", reconnect_count)
        global FLAG_EXIT
        FLAG_EXIT = True



    def on_message1(self, client, userdata, message):
        if str( message.payload.decode("utf-8") ) == "reset":
            self.allCupsHit1 = False
            self.allCupsHit2 = False
            self.hitCups1 = [0,0,0,0,0,0]
            self.hitCups2 = [0,0,0,0,0,0]
            self.team1score_int = 0
            self.team1score = str(self.team1score_int)
            self.team2score_int = 0
            self.team2score = str(self.team2score_int)
            return
        binTmp = str( bin( int( str( message.payload.decode("utf-8") ) ) )[2:] )

        while len(binTmp) < 6:
            binTmp="0"+binTmp

        
        
        for i in range(len(self.hitCups1)):

            if self.hitCups1[i] == 0 and int(binTmp[i]) == 1:
                self.hitCups1[i] = 1
                self.team1score_int += 1

        self.team1score = str(self.team1score_int)

    def on_message2(self, client, userdata, message):
        #print("message received " ,str(message.payload.decode("utf-8")))
        if str( message.payload.decode("utf-8") ) == "reset":
            self.allCupsHit1 = False
            self.allCupsHit2 = False
            self.hitCups1 = [0,0,0,0,0,0]
            self.hitCups2 = [0,0,0,0,0,0]
            self.team1score_int = 0
            self.team1score = str(self.team1score_int)
            self.team2score_int = 0
            self.team2score = str(self.team2score_int)
            return

        binTmp = str( bin( int( str( message.payload.decode("utf-8") ) ) )[2:] )

        while len(binTmp) < 6:
            binTmp="0"+binTmp
        
        for i in range(len(self.hitCups2)):

            if self.hitCups2[i] == 0 and int(binTmp[i]) == 1:
                self.hitCups2[i]=1
                self.team2score_int += 1

        self.team2score = str(self.team2score_int)


    def run(self):
        client1 = mqtt.Client("LED-Matrix-T1")
        client1.username_pw_set("pi", "raspberry")
        
        client1.on_connect = self.on_connect1
        client1.on_message = self.on_message1
        client1.connect("192.168.24.1", 1883, keepalive=120)
        client1.on_disconnect = self.on_disconnect

        client2 = mqtt.Client("LED-Matrix-T2")
        client2.username_pw_set("pi", "raspberry")
        client2.on_connect = self.on_connect2
        client2.on_message = self.on_message2
        client2.connect("192.168.24.1", 1883, keepalive=120)
        client2.on_disconnect = self.on_disconnect


        client1.loop_start()
        client2.loop_start()


        ########################################
        # #broker_address="192.168.1.184"
        # #broker_address="iot.eclipse.org"
        # print("creating new instance")
        # client = mqtt.Client("P1") #create new instance
        # client.on_message=self.on_message #attach function to callback
        # print("connecting to broker")
        # client.username_pw_set("pi", "raspberry")
        # client.connect("192.168.24.1", 1883) #connect to broker
        # client.loop_start() #start the loop
        # print("Subscribing to topic","team/1/score")
        # client.subscribe("team/1/score")

        path = os.path.dirname(os.path.abspath(__file__))


        offscreen_canvas = self.matrix.CreateFrameCanvas()
        font_team1 = graphics.Font()
        font_team1.LoadFont("../../../fonts/4x6.bdf")
        font_team2 = graphics.Font()
        font_team2.LoadFont("../../../fonts/4x6.bdf")
        team1Color = graphics.Color(255, 0, 0)
        team2Color = graphics.Color(0, 0, 255)
        pos = offscreen_canvas.width


        font_score = graphics.Font()
        font_score.LoadFont("../../../fonts/9x18B.bdf")
        font_finished = graphics.Font()
        font_finished.LoadFont("../../../fonts/7x14B.bdf")
        team1 = "Team 1"
        team2 = "Team 2"

        team1Score = "0"
        team2Score = "0"

        offset_canvas = self.matrix.CreateFrameCanvas()

        while True:
            offscreen_canvas.Clear()
            team1Score = self.team1score
            team2Score = self.team2score
            #print(self.team1score,self.team2score)
            graphics.DrawText(offscreen_canvas, font_team1, 5, 8, team1Color, team1)
            graphics.DrawText(offscreen_canvas, font_team2, 37, 8, team2Color, team2)
            graphics.DrawText(offscreen_canvas, font_score, 12, 26, graphics.Color(0, 255, 0), team1Score)
            graphics.DrawText(offscreen_canvas, font_score, 44, 26, graphics.Color(0, 255, 0), team2Score)
            for x in range(0, self.matrix.width):
                offscreen_canvas.SetPixel(x, 0, 255, 255, 255)

                offscreen_canvas.SetPixel(0, x, 255, 255, 255)
                offscreen_canvas.SetPixel(x, 10, 255, 255, 255)
                offscreen_canvas.SetPixel(x, offscreen_canvas.height-1, 255, 255, 255)
                offscreen_canvas.SetPixel(offscreen_canvas.width-1, x, 255, 255, 255)

                offscreen_canvas.SetPixel(offscreen_canvas.width / 2, x, 255, 255, 255)
                #offscreen_canvas.SetPixel(offscreen_canvas.width / 2 - 1, x, 255, 255, 255)

            if ((self.hitCups1[0] == self.hitCups1[1] == self.hitCups1[2] == self.hitCups1[3] == self.hitCups1[4] == self.hitCups1[5]  == 1) and self.allCupsHit2 == False):
                #print("Finished_round")
                self.allCupsHit1 = True

            if ((self.hitCups2[0] == self.hitCups2[1] == self.hitCups2[2] == self.hitCups2[3] == self.hitCups2[4] == self.hitCups2[5]  == 1) and self.allCupsHit1 == False):
                #print("Finished_round")
                self.allCupsHit2 = True
            
            if (self.allCupsHit1 == True ):
            
                for x in range(0, self.matrix.width):
                    for y in range(0, self.matrix.height):
                        offscreen_canvas.SetPixel(x, y, 0, 0, 0)

                graphics.DrawText(offscreen_canvas, font_finished, 12, 15, graphics.Color(255, 0, 0), "Team 1")
                graphics.DrawText(offscreen_canvas, font_finished, 20, 28, graphics.Color(255, 0, 0), "Won!")

                            
            if (self.allCupsHit2 == True ):
            
                for x in range(0, self.matrix.width):
                    for y in range(0, self.matrix.height):
                        offscreen_canvas.SetPixel(x, y, 0, 0, 0)



                graphics.DrawText(offscreen_canvas, font_finished, 12, 15, graphics.Color(0, 0, 255), "Team 2")
                graphics.DrawText(offscreen_canvas, font_finished, 20, 28, graphics.Color(0, 0, 255), "Won!")

            time.sleep(0.05)
            offscreen_canvas = self.matrix.SwapOnVSync(offscreen_canvas)
        client1.stop_loop()
        client2.stop_loop()



# Main function
if __name__ == "__main__":
    run_text = RunText()
    if (not run_text.process()):
        run_text.print_help()
