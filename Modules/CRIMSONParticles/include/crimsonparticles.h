#pragma once

#include <string>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/trim.hpp>

#include <iostream>

namespace crimsonparticles {

	enum class ReturnCode {
		Ok = 0,
		Error = -1,
		NotAvailable = -2,
		NotRunning = -3,
		JsonNotFound = -4,
		CwdWriteFailure = -5,
		RequestedTooManyCpuCores = -6,
		StatusRetrievalFailed = -7
	};

	__declspec(dllexport) ReturnCode get_exe_return_status(const std::string config_filename);

	inline bool error_occurred(const ReturnCode code)
	{
		if (code != ReturnCode::Ok)
		{
			return true;
		}
		else
		{
			return false;
		}
	}

	inline void abort_if_error(const ReturnCode code, const double error_code) {
		if (error_occurred(code))
		{
			std::cerr << "Failed with error code " << error_code << ". Please contact the developers." << std::endl;
			std::exit(-1);
		}
	}
	
	const bool FLUID_PROBLEM = true;
	const bool BOLUS_OR_BIN = false;

	__declspec(dllexport) void print_welcome_message();

	__declspec(dllexport) ReturnCode is_available();
	__declspec(dllexport) ReturnCode check_system(const std::string config_filename);

	__declspec(dllexport) ReturnCode remove_existing_logfile(const std::string config_filename);

	__declspec(dllexport) ReturnCode run_multivispostsolver(const std::string config_filename);
	__declspec(dllexport) ReturnCode run_mesh_extract(const std::string config_filename);
	__declspec(dllexport) ReturnCode write_particle_config_json_for_mesh_extraction(const std::string config_filename, const bool is_for_fluid_problem);
	__declspec(dllexport) ReturnCode extract_fluid_mesh(const std::string config_filename);
	__declspec(dllexport) ReturnCode convert_fluid_sim_to_hdf5(const std::string config_filename);
	__declspec(dllexport) ReturnCode extract_particle_mesh(const std::string config_filename);
	__declspec(dllexport) ReturnCode extract_bin_meshes(const std::string config_filename);
	__declspec(dllexport) ReturnCode run_particle_tracking(const std::string config_filename);
	__declspec(dllexport) ReturnCode emplace_particle_config_json(const std::string config_filename);
	__declspec(dllexport) ReturnCode is_docker_server_running(const std::string config_filename);
	__declspec(dllexport) ReturnCode extract_final_data(const std::string config_filename);
	__declspec(dllexport) ReturnCode finish(const std::string config_filename);

}