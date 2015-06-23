
#include <iostream>
#include <iterator>
#include "map.h"
#include "ray.h"
#include "newray.h"
#include "mcdmfunction.h"
#include "graphpose.h"
#include "Criteria/traveldistancecriterion.h"
# define PI           3.14159265358979323846  /* pi */
#include <unistd.h>



using namespace std;
using namespace dummy;

int main(int argc, char **argv) {
    
    // Input : ./mcdm_online_exploration_ros ./../Maps/map_RiccardoFreiburg_1m2.pgm 100 75 5 0 15 180 0.95 0.12
    // resolution x y orientation range centralAngle precision threshold

    ifstream infile;
    infile.open(argv[1]);
    int resolution = atoi(argv[2]);
    //ifstream infile("/home/pulver/Dropbox/Università/Laurea Magistrale/Thesis/testmap10.pgm");
    Map map = Map(infile,resolution);
    cout << "Map dimension: " << map.getNumGridCols() << " : "<<  map.getNumGridRows() << endl;
    // Pose initialPose = map.getRobotPosition();
    
    // i switched x and y because the map's orientation inside and outside programs are different
    long  initX = atoi(argv[4]);	
    long initY = atoi(argv[3]);
    int initOrientation = atoi(argv[5]);
    double initFov = atoi(argv[7] );
    initFov = initFov * PI /180;
    int initRange = atoi(argv[6]);
    double precision = atof(argv[8]);
    double threshold = atof(argv[9]);
    //x,y,orientation,range,angle -
    Pose initialPose = Pose(initX,initY,initOrientation,initRange,initFov);
    Pose p1 = Pose(initX,initY,0,initRange,initFov);
    Pose p2 = Pose(initX,initY,90,initRange,initFov);
    Pose p3 = Pose(initX,initY,180,initRange,initFov);
    Pose p4 = Pose(initX,initY,270,initRange,initFov);
    Pose target = initialPose;
    Pose previous = initialPose;
    long numConfiguration =0;
    //testing
    vector<pair<string,list<Pose>>> graph2;
    NewRay ray;
    MCDMFunction function;
    long sensedCells = 0;
    long newSensedCells =0;
    long totalFreeCells = map.getTotalFreeCells() ;
    int count = 0;
    double travelledDistance = 0;
    unordered_map<string,int> visitedCell;
    vector<string>history;
    history.push_back(function.getEncodedKey(target,1));
    //amount of time the robot should do nothing for scanning the environment ( final value expressed in second)
    unsigned int microseconds = 5 * 1000 * 1000 ;
    //cout << "total free cells in the main: " << totalFreeCells << endl;
    list<Pose> unexploredFrontiers;
    
    while(sensedCells < precision * totalFreeCells ){
	long x = target.getX();
	long y = target.getY();
	int orientation = target.getOrientation();
	int range = target.getRange();
	double FOV = target.getFOV();
	string actualPose = function.getEncodedKey(target,0);
	map.setCurrentPose(target);
	travelledDistance = travelledDistance + target.getDistance(previous);
	string encoding = to_string(target.getX()) + to_string(target.getY());
	visitedCell.emplace(encoding,0);
	
	
	
	cout << "-----------------------------------------------------------------"<<endl;
	cout << "Round : " << count<< endl;
	newSensedCells = sensedCells + ray.getInformationGain(map,x,y,orientation,FOV,range);
	cout << "Area sensed: " << newSensedCells << " / " << totalFreeCells<< endl;

	ray.performSensingOperation(map,x,y,orientation,FOV,range);
	ray.findCandidatePositions(map,x,y,orientation,FOV,range);
	vector<pair<long,long> >candidatePosition = ray.getCandidatePositions();
	ray.emptyCandidatePositions();
	
	/*
	//NOTE: PRINT THE MAP NEAR THE ROBOT-------------
	int curX = previous.getX();
	int curY = previous.getY();
	int minX = curX - 30;
	if(minX < 0) minX = 0;
	int maxX = curX + 30;
	if(maxX > map.getNumGridRows()-1) maxX= map.getNumGridRows()-1;
	int minY = curY - 30;
	if(minY < 0) minY = 0;
	int maxY = curY + 30;
	if(maxY > map.getNumGridCols()-1) maxY = map.getNumGridCols()-1;
	//print portion of the map
	    for(int i = minX; i < maxX; ++i)
	    {
		for(int j = minY; j < maxY; ++j)
		{
		   /* if(i == curX && j == curY)
		    {
			//std::cout << "X ";
		    }
		    else if (i == target.getX() && j == target.getY())
		    {
			std::cout << "Y ";
		    }
		    else std::cout << map.getGridValue(i, j) << " ";
		}
		std::cout << std::endl;
	    }
	    std::cout << std::endl;
	//---------------------------------------
	*/
	
	if(candidatePosition.size() == 0) {
	    
	    cout << "No other candidate position" << endl;
	    cout << "----- BACKTRACKING -----" << endl;
	    
	    
	    
	    if (graph2.size() >0){
		
		// OLD METHOD
		string targetString = graph2.at(graph2.size()-1).first;
		graph2.pop_back();
		
		EvaluationRecords record;
		target = record.getPoseFromEncoding(targetString);
		history.push_back(function.getEncodedKey(target,2));
		cout << "[BT]No significative position reachable. Come back to previous position" << endl;
		cout << "New target: x = " << target.getY() << ",y = " << target.getX() <<", orientation = " << target.getOrientation() << endl;
		count = count + 1;
		cout << "Graph dimension : " << graph2.size() << endl;
		
	    } else {
		cout << "-----------------------------------------------------------------"<<endl;
		cout << "I came back to the original position since i don't have any other candidate position"<< endl;
		cout << "Total cell visited :" << numConfiguration <<endl;
		cout << "FINAL: Map not completely explored!" << endl;
		cout << "-----------------------------------------------------------------"<<endl;
		exit(0);
	    }
	   
	}else{
	    
	    
	    // need to convert from a <int,int pair> to a Pose with also orientation,laser range and angle
	    list<Pose> frontiers;
	    vector<pair<long,long> >::iterator it =candidatePosition.begin();
	    for(it; it != candidatePosition.end(); it++){
		Pose p1 = Pose((*it).first,(*it).second,0 ,range,FOV);
		Pose p2 = Pose((*it).first,(*it).second,180,range,FOV);
		Pose p3 = Pose((*it).first,(*it).second,90,range,FOV);
		Pose p4 = Pose((*it).first,(*it).second,270,range,FOV);
		frontiers.push_back(p1);
		frontiers.push_back(p2);
		frontiers.push_back(p3);
		frontiers.push_back(p4);
	    }
	    
	    unexploredFrontiers = frontiers;
	    
	    cout << "Graph dimension : " << graph2.size() << endl;
	    cout << "Candidate position: " << candidatePosition.size() << endl;
	    cout <<"Frontiers: "<<  frontiers.size() << endl;
	    EvaluationRecords *record = function.evaluateFrontiers(frontiers,map,threshold);
	    //cout << "Record: " << record->size() << endl;
	    cout << "Evaluation Record obtained" << endl;
	    
	    
	    if(record->size() != 0){
		//set the previous pose equal to the actual one(actually represented by target)
		previous = target;
		std::pair<string,list<Pose>> pair = make_pair(actualPose,frontiers);
		graph2.push_back(pair);
		std::pair<Pose,double> result = function.selectNewPose(record);
		target = result.first;
		if (!target.isEqual(previous)){
		    count = count + 1;
		    numConfiguration++;
		    history.push_back(function.getEncodedKey(target,1));
		    cout << "Graph dimension : " << graph2.size() << endl;
		    //cout << record->size() << endl;
		}else{
		    cout << "[BT]Cell already explored!Come back to previous position";
		    
		    string targetString = graph2.at(graph2.size()-2).first;
		    target = record->getPoseFromEncoding(targetString);
		    graph2.pop_back();
		    history.push_back(function.getEncodedKey(target,2));
		    cout << "New target: x = " << target.getY() << ",y = " << target.getX() <<", orientation = " << target.getOrientation() << endl;
		    count = count + 1;
		    cout << "Graph dimension : " << graph2.size() << endl;
		}
	    }else {  
		    //OLD METHOD
		    
		    if(graph2.size() == 0 ) break;
		    
		    string targetString = graph2.at(graph2.size()-1).first;
		    graph2.pop_back();
		    target = record->getPoseFromEncoding(targetString);
		    
		    if(!target.isEqual(previous)){
			previous = target;
			cout << "[BT]No significative position reachable. Come back to previous position" << endl;
			history.push_back(function.getEncodedKey(target,2));
			cout << "New target: x = " << target.getY() << ",y = " << target.getX() <<", orientation = " << target.getOrientation() << endl;
			count = count + 1;
			cout << "Graph dimension : " << graph2.size() << endl;
			
		    }else {
			
			if(graph2.size() == 0 ) {
			    cout << "[BT]No other possibilities to do backtracking on previous positions" << endl;
			    break;
			}
			string targetString = graph2.at(graph2.size()-1).first;
			graph2.pop_back();
			target = record->getPoseFromEncoding(targetString);
			previous = target;
			cout << "[BT]No significative position reachable from the previous position. Come back to another previous one" << endl;
			history.push_back(function.getEncodedKey(target,2));
			cout << "New target: x = " << target.getY() << ",y = " << target.getX() <<", orientation = " << target.getOrientation() << endl;
			count = count + 1;
			cout << "Graph dimension : " << graph2.size() << endl;
		    }
		    
	    }
    
	    
	    
	    //NOTE: not requested for testing purpose
	    //usleep(microseconds);
	    
	    frontiers.clear();
	    candidatePosition.clear();
	    delete record;
	}
	
	sensedCells = newSensedCells;
    }
    
    map.drawVisitedCells(visitedCell,resolution);
    map.printVisitedCells(history);
  
   
    //OLD METHOD
    
    if (graph2.size() != 0 && sensedCells >= precision * totalFreeCells ){
	cout << "-----------------------------------------------------------------"<<endl;
	cout << "Total cell visited :" << numConfiguration <<endl;
	cout << "Total travelled distance (cells): " << travelledDistance << endl;
	cout << "FINAL: MAP EXPLORED!" << endl;
	cout << "-----------------------------------------------------------------"<<endl;
    }else{
	cout << "-----------------------------------------------------------------"<<endl;
	cout << "I came back to the original position since i don't have any other candidate position"<< endl;
	cout << "Total cell visited :" << numConfiguration <<endl;
	cout << "-----------------------------------------------------------------"<<endl;
	
    }
    
}
