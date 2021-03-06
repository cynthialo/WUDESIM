/*
Project:     WUDESIM ver. 1 BETA
File:        OpenEPANETinp.cpp
Author:      Ahmed Abokifa
Date:        10/25/2016
Description: This function imports EPANET input file specified by the user, reads all the data for network elements including pipes, 
             junctions, tanks, reservoirs, pumps, and valves, and reads all the simulation parameters for times, reactions, and options.
*/

#include <iostream> 
#include <fstream>  
#include <vector>  
#include <map>
#include <algorithm>
#include <sstream>  
#include <string>  
#include <iterator>
#include <numeric>
#include <stdio.h>
#include <tchar.h>
#include <SDKDDKVer.h>

#include "Classes.h"
#include "WUDESIMmain.h"
#include "Utilities.h"

using namespace std;

int OpenEPANETinp(string INPfileName, Network* net)
{

	// Import EPANET input file (.inp)
	vector<string> EPANETinp;
	EPANETinp = ImportFile(INPfileName);
	
	if (EPANETinp.empty()) {
		cout << INPfileName << "is empty/corrupt!" << endl;
		return 1;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	// Find section locations

	vector<int> index;
	vector<string> Headers = {
		"[TITLE]","[JUNCTIONS]","[RESERVOIRS]","[TANKS]","[PIPES]","[PUMPS]","[VALVES]","[EMITTERS]",
		"[CURVES]","[PATTERNS]","[ENERGY]","[STATUS]","[CONTROLS]","[RULES]","[DEMANDS]",
		"[QUALITY]","[REACTIONS]","[SOURCES]","[MIXING]",
		"[OPTIONS]","[TIMES]","[REPORT]",
		"[COORDINATES]","[VERTICES]","[LABELS]","[BACKDROP]","[TAGS]",
		"[END]" };

	for (int j = 0;j < Headers.size();++j) {
		for (int i = 0;i < EPANETinp.size();++i) { if (find(Headers[j], EPANETinp[i])) { index.push_back(i); } };
	}
	sort(index.begin(), index.end());

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	// Read Pipes data from input file

	vector<string> pipe_data;
	pipe_data = InputData(index, EPANETinp, "[PIPES]");

	int N_pipes = pipe_data.size();    //Number of pipes
	
	if (N_pipes == 0) {
		return 1;
	}
	else {
		net->pipes.resize(N_pipes);

		for (int i = 0;i < N_pipes;++i) {
			istringstream iss(pipe_data[i]);
			iss >> net->pipes[i].id >> net->pipes[i].node_1 >> net->pipes[i].node_2 >> net->pipes[i].length >> net->pipes[i].diameter;
		}
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	// Read Junctions data from input file

	vector<string> node_data;
	node_data = InputData(index, EPANETinp, "[JUNCTIONS]");

	int N_nodes = node_data.size();
	
	if (N_nodes == 0) {
		return 1;
	}
	else {
		net->junctions.resize(N_nodes);

		for (int i = 0;i < N_nodes;++i) {
			istringstream iss(node_data[i]);
			iss >> net->junctions[i].id >> net->junctions[i].elev >> net->junctions[i].demand;
			if (net->junctions[i].demand < 0)
			{
				net->sources.push_back(net->junctions[i].id);
			}
		}
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	//Read Tanks data from input file

	vector<string> tanks_data;
	tanks_data = InputData(index, EPANETinp, "[TANKS]");

	int N_tanks = tanks_data.size();

	net->tanks.resize(N_tanks);
	for (int i = 0;i < N_tanks;++i) {
		istringstream iss(tanks_data[i]);
		iss >> net->tanks[i];
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	//Read Reservoirs data from input file
	vector<string> reserv_data;
	reserv_data = InputData(index, EPANETinp, "[RESERVOIRS]");

	int N_reserv = reserv_data.size();

	net->reservoirs.resize(N_reserv);

	for (int i = 0;i < N_reserv;++i) {
		istringstream iss(reserv_data[i]);
		iss >> net->reservoirs[i];
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	//Read Pumps data from input file

	vector<string> pumps_data;
	pumps_data = InputData(index, EPANETinp, "[PUMPS]");

	int N_pumps = pumps_data.size();

	net->pumps.resize(N_pumps);

	for (int i = 0;i < N_pumps;++i) {
		istringstream iss(pumps_data[i]);
		iss >> net->pumps[i].id >> net->pumps[i].start >> net->pumps[i].end;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	//Read Valves data from input file

	vector<string> valves_data;
	valves_data = InputData(index, EPANETinp, "[VALVES]");

	int N_valves = valves_data.size();

	net->valves.resize(N_valves);

	for (int i = 0;i < N_valves;++i) {
		istringstream iss(valves_data[i]);
		iss >> net->valves[i].id >> net->valves[i].start >> net->valves[i].end;
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	// Read Options data from input file
	string dummy;

	vector<string> options_data;
	options_data = InputData(index, EPANETinp, "[OPTIONS]");

	for (int i = 0;i < options_data.size();++i) {
		istringstream iss(options_data[i]);

		if (find("Viscosity", options_data[i])) { iss >> dummy >> net->options.Rel_Viscosity; }
		if (find("Diffusivity", options_data[i])) { iss >> dummy >> net->options.Rel_Diffusivity; }
		if (find("UNITS", options_data[i])) { iss >> dummy >> net->options.Flow_UNITS; }
		if (find("QUALITY", options_data[i])) { iss >> dummy >> net->options.QUAL_TAG>> net->options.QUAL_UNIT; }
	}

	if (find("NONE", net->options.QUAL_TAG) || find("AGE", net->options.QUAL_TAG) || find("TRACE", net->options.QUAL_TAG)) {
		cout << "WUDESIM can only take CHEMICAL water quality analysis" << endl;
		return 1;
	}
	
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	// Read Times data from input file

	vector<string> times_data;
	times_data = InputData(index, EPANETinp, "[TIMES]");

	char dummy1;

	for (int i = 0;i < times_data.size();++i) {
		istringstream iss(times_data[i]);

		// Read duration
		if (find("Duration", times_data[i])) { iss >> dummy >> net->times.Duration_hr >> dummy1 >> net->times.Duration_min; goto nexttime; }
		
		// Read Hydraulic Time step
		if (find("Hydraulic Timestep", times_data[i])) { iss >> dummy >> dummy >> net->times.Hyd_step_hr >> dummy1 >> net->times.Hyd_step_min; goto nexttime; }

		// Read Quality Time step
		if (find("Quality Timestep", times_data[i])) { iss >> dummy >> dummy >> net->times.Qual_step_hr >> dummy1 >> net->times.Qual_step_min;goto nexttime; }
				
		// Read Report Time step
		if (find("Report Timestep", times_data[i])) { iss >> dummy >> dummy >> net->times.Rep_step_hr >> dummy1 >> net->times.Rep_step_min;goto nexttime; }

		// Read Report Start
		if (find("Report Start", times_data[i])) { iss >> dummy >> dummy >> net->times.Rep_start_hr >> dummy1 >> net->times.Rep_start_min;goto nexttime; }
	nexttime:;
	}

	if (net->times.Duration_hr + net->times.Duration_min == 0) { cout << "WUDESIM can't run single period snapshot analysis" << endl; return 1; }


	if (net->times.Qual_step_hr + net->times.Qual_step_min == 0) {
		net->times.Qual_step_min = floor((net->times.Hyd_step_hr * 60 + net->times.Hyd_step_min) / 10);
	}

	net->times.N_steps = ((net->times.Duration_hr - net->times.Rep_start_hr) * 60 + (net->times.Duration_min - net->times.Rep_start_min)) / (net->times.Rep_step_hr * 60 + net->times.Rep_step_min) + 1;
	if (net->times.N_steps <= 0) { return 1; }

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	
	// Read Global Reactions data from input file

	vector<string> reactions_data;
	reactions_data = InputData(index, EPANETinp, "[REACTIONS]");

	for (int i = 0;i < reactions_data.size();++i) {
		istringstream iss(reactions_data[i]);

		if (find("Global Bulk", reactions_data[i])) { iss >> dummy >> dummy >> net->reactions.Bulk_coeff; }  // Read Bulk Coeff (/day)
		if (find("Global Wall", reactions_data[i])) { iss >> dummy >> dummy >> net->reactions.Wall_coeff; }  // Read Wall Coeff (length/day)
		if (find("Order Bulk", reactions_data[i])) { iss >> dummy >> dummy >> net->reactions.Bulk_order; }
		if (find("Order Wall", reactions_data[i])) { iss >> dummy >> dummy >> net->reactions.Wall_order; }
		if (find("Limiting Potential", reactions_data[i])) { iss >> dummy >> dummy >> net->reactions.Lim_pot; }
	}
	
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	//Flow Unit Conversion to m3/sec
	if      (net->options.Flow_UNITS == "CFS") { net->options.Flow_unit_conv = 0.028316847;          net->options.unit_sys = 0; }
	else if (net->options.Flow_UNITS == "GPM") { net->options.Flow_unit_conv = 0.0000630901964;      net->options.unit_sys = 0; }
	else if (net->options.Flow_UNITS == "MGD") { net->options.Flow_unit_conv = 0.043812636574;       net->options.unit_sys = 0; }
	else if (net->options.Flow_UNITS == "IMGD"){ net->options.Flow_unit_conv = 0.052616782407;       net->options.unit_sys = 0; }
	else if (net->options.Flow_UNITS == "AFD") { net->options.Flow_unit_conv = 0.014276410185;       net->options.unit_sys = 0; }
	else if (net->options.Flow_UNITS == "LPS") { net->options.Flow_unit_conv = 0.001;                net->options.unit_sys = 1; }
	else if (net->options.Flow_UNITS == "LPM") { net->options.Flow_unit_conv = 0.000016666666667;    net->options.unit_sys = 1; }
	else if (net->options.Flow_UNITS == "MLD") { net->options.Flow_unit_conv = 0.0115740741;         net->options.unit_sys = 1; }
	else if (net->options.Flow_UNITS == "CMH") { net->options.Flow_unit_conv = 0.00027777777778;     net->options.unit_sys = 1; }
	else if (net->options.Flow_UNITS == "CMD") { net->options.Flow_unit_conv = 0.000011574074074;    net->options.unit_sys = 1; }

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	// Pipe length and diameter conversion
	for (int i = 0;i < net->pipes.size();++i) {
		if (net->options.unit_sys == 0) {
			net->pipes[i].length *= 0.3048;     //ft-->m
			net->pipes[i].diameter *= 0.0254;   //in-->m
		}
		else {
			net->pipes[i].length *= 1;         //m-->m
			net->pipes[i].diameter *= 0.001;   //mm-->m
		}
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	// Reaction Unit Conversion
	// Bulk
	net->reactions.Bulk_coeff *= 1 / (24 * 3600); // 1/day --> 1/sec
												  // Wall
	if (net->options.unit_sys == 0) {
		if (net->reactions.Wall_order == 1) { net->reactions.Wall_coeff *= 0.3048 / (24 * 3600); }                       // ft/day --> m/sec
		else if (net->reactions.Wall_order == 0) { net->reactions.Wall_coeff /= (pow(0.3048, 2) * 24 * 3600); }        // 1/ft2/day --> 1/m2/sec
	}
	else if (net->options.unit_sys == 1) { net->reactions.Wall_coeff *= 1 / (24 * 3600); }  // m/day --> m/sec

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	return 0;

}