#include <uWS/uWS.h>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include "json.hpp"
#include "BehaviourPlanner.h"
#include "TrajectoryPlanner.h"
#include "HighwayMap.h"

// for convenience
using nlohmann::json;
using std::string;
using std::vector;

int main() 
{

    uWS::Hub h;

    // Waypoint map to read from
    // The max s value before wrapping around the track back to 0

    HighwayMap* hMap = HighwayMap::getInstance();
    BehaviourPlanner b(hMap);
    TrajectoryPlanner t(hMap);
    car carCurr(&b, &t);


     h.onMessage([&b, &t, &carCurr, hMap]
              (uWS::WebSocket<uWS::SERVER> ws, char *data, size_t length,
               uWS::OpCode opCode) 
         {
             // "42" at the start of the message means there's a websocket message event.
             // The 4 signifies a websocket message
             // The 2 signifies a websocket event
             if (length && length > 2 && data[0] == '4' && data[1] == '2') {

                 auto s = hasData(data);

                 if (s != "") 
                 {
                     auto j = json::parse(s);
                     string event = j[0].get<string>();

                        if (event == "telemetry") {
                            // j[1] is the data JSON object

                            // Main car's localization Data
                             // j[1] is the data JSON object

                            // Main car's localization Data
                            double car_x = j[1]["x"];
                            double car_y = j[1]["y"];
                            double car_s = j[1]["s"];
                            double car_d = j[1]["d"];
                            double car_yaw = j[1]["yaw"];
                            double car_speed = j[1]["speed"];

                            // Previous path data given to the Planner
                            auto previous_path_x = j[1]["previous_path_x"];
                            auto previous_path_y = j[1]["previous_path_y"];
                            // Previous path's end s and d values 
                            double end_path_s = j[1]["end_path_s"];
                            double end_path_d = j[1]["end_path_d"];
                            

                            // Sensor Fusion Data, a list of all other cars on the same side 
                            //   of the road.
                            vector<vector<double>> sensor_fusion = j[1]["sensor_fusion"];


                            /**
                             * TODO: define a path made up of (x,y) points that the car will visit
                             *   sequentially every .02 seconds
                            */

                            //START ----------------------
                            carCurr.setValues(car_x, car_y, car_s, car_d, deg2rad(car_yaw), (car_d / 4), car_speed);
                            
                            // std::cout << " Car values: " << carCurr._s << "," << carCurr._d << "," << carCurr._speed << "," << carCurr._lane << "," << std::endl;

                            /*
                            // cout << endl << "Created behaviour planner" << endl;
                            carCurr._tp->setEnvironment(carCurr, previous_path_x.size(), sensor_fusion);
                            pair<double, int> next =  carCurr._tp->returnNextAction();
                            */
                            // cout << endl << "Creating trajectory planner" << endl;
                            vector<vector<double>> nextTraj = carCurr._tp->generateTrajectory
                            (previous_path_x, previous_path_y, 
                                carCurr, 
                                10, 1, end_path_s);
                            
                            
                            //END ------------------------
                            json msgJson;

                           
                            msgJson["next_x"] = nextTraj[0];
                            msgJson["next_y"] = nextTraj[1];

                            auto msg = "42[\"control\"," + msgJson.dump() + "]";

                            ws.send(msg.data(), msg.length(), uWS::OpCode::TEXT);
                        }  // end "telemetry" if
                    }
                    else {
                        // Manual driving
                        std::string msg = "42[\"manual\",{}]";
                        ws.send(msg.data(), msg.length(), uWS::OpCode::TEXT);
                    }
                }  // end websocket if
        }); // end h.onMessage

   

    h.onConnection([&h](uWS::WebSocket<uWS::SERVER> ws, uWS::HttpRequest req) {
        std::cout << "Connected!!!" << std::endl;
        });

    h.onDisconnection([&h](uWS::WebSocket<uWS::SERVER> ws, int code,
        char* message, size_t length) {
            ws.close();
            std::cout << "Disconnected" << std::endl;
        });

    int port = 4567;
    if (h.listen(port)) {
        std::cout << "Listening to port " << port << std::endl;
    }
    else {
        std::cerr << "Failed to listen to port" << std::endl;
        return -1;
    }

    h.run();
}



