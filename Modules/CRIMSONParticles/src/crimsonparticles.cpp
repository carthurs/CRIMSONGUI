#include "pch.h"

#include <iostream>
#include <ShellApi.h>

#include "crimsonparticles.h"

#define PARTICLE_TRACKING_EXECUTABLE_PATH "bin/CRIMSON_particles.exe"

namespace crimsonparticles {

	ReturnCode get_exe_return_status(const std::string config_filename) {
		boost::filesystem::path status_file_path_with_json{ config_filename };
		boost::filesystem::path status_file_path = status_file_path_with_json.parent_path();
		status_file_path /= boost::filesystem::path("status.stat");

		if (!boost::filesystem::exists(status_file_path))
		{
			std::cerr << "couldn't get " << status_file_path << std::endl;
			return ReturnCode::StatusRetrievalFailed;
		}

		boost::filesystem::ifstream status_file_stream(status_file_path);
		std::string return_code_string;
		status_file_stream >> return_code_string;
		boost::algorithm::trim(return_code_string);
		status_file_stream.close();

		std::cout << "Particle status: " << return_code_string << std::endl;

		int return_code_int = boost::lexical_cast<int>(return_code_string);

		boost::filesystem::remove(status_file_path);

		return static_cast<ReturnCode>(return_code_int);
	}

	void print_welcome_message()
	{
		std::cout << "Particles initializing..." << std::endl;
	}

	std::string escape_spaces(const std::string& string_to_escape)
	{
		std::stringstream escaped_string_builder;
		for (auto character_iterator = string_to_escape.begin(); character_iterator != string_to_escape.end(); character_iterator++)
		{
			if (*character_iterator == ' ')
			{
				escaped_string_builder << '`';
			}
			escaped_string_builder << *character_iterator;
		}

		return escaped_string_builder.str();
	}

	void run_shell_command(const std::string command)
	{
		SHELLEXECUTEINFOA shell_execute_info = { 0 };
		shell_execute_info.cbSize = sizeof(SHELLEXECUTEINFOA);
		shell_execute_info.fMask = SEE_MASK_NOCLOSEPROCESS; // allows hProcess to receive process handle, which it will use to wait for the shell to terminate
		shell_execute_info.lpFile = "powershell.exe";

		shell_execute_info.lpParameters = command.c_str();
		shell_execute_info.nShow = SW_SHOW;

		ShellExecuteExA(&shell_execute_info);
		WaitForSingleObject(shell_execute_info.hProcess, INFINITE);
		CloseHandle(shell_execute_info.hProcess);
	}

	ReturnCode is_available()
	{
		return ReturnCode::Ok;
	}

	ReturnCode check_system(const std::string config_filename)
	{
		std::stringstream command;
		command << PARTICLE_TRACKING_EXECUTABLE_PATH << " --single-action check_system " << escape_spaces(config_filename) << std::endl;
		run_shell_command(command.str());
		return get_exe_return_status(config_filename);
	}

	ReturnCode remove_existing_logfile(const std::string config_filename)
	{
		std::stringstream command;
		command << PARTICLE_TRACKING_EXECUTABLE_PATH << " --single-action remove_existing_logfile " << escape_spaces(config_filename) << std::endl;
		run_shell_command(command.str());
		return get_exe_return_status(config_filename);
	}

	ReturnCode run_multivispostsolver(const std::string config_filename)
	{
		std::stringstream command;
		command << PARTICLE_TRACKING_EXECUTABLE_PATH << " --single-action run_multivispostsolver " << escape_spaces(config_filename) << std::endl;
		run_shell_command(command.str());
		return get_exe_return_status(config_filename);
	}


	ReturnCode run_mesh_extract(const std::string config_filename)
	{
		std::stringstream command;
		command << PARTICLE_TRACKING_EXECUTABLE_PATH << " --single-action run_mesh_extract " << escape_spaces(config_filename) << std::endl;
		run_shell_command(command.str());
		return get_exe_return_status(config_filename);
	}


	ReturnCode write_particle_config_json_for_mesh_extraction(const std::string config_filename, const bool is_for_fluid_problem)
	{
		std::stringstream command;
		command << PARTICLE_TRACKING_EXECUTABLE_PATH << " --single-action write_particle_config_json_for_mesh_extraction " << escape_spaces(config_filename);
		
		if (is_for_fluid_problem) {
			command << " --setup-fluid-mesh-extraction";
		}
		command << std::endl;

		run_shell_command(command.str());
		return get_exe_return_status(config_filename);
	}


	ReturnCode extract_fluid_mesh(const std::string config_filename)
	{
		std::stringstream command;
		command << PARTICLE_TRACKING_EXECUTABLE_PATH << " --single-action extract_fluid_mesh " << escape_spaces(config_filename) << std::endl;
		run_shell_command(command.str());
		return get_exe_return_status(config_filename);
	}

	ReturnCode extract_bin_meshes(const std::string config_filename)
	{
		std::stringstream command;
		command << PARTICLE_TRACKING_EXECUTABLE_PATH << " --single-action extract_bin_meshes " << escape_spaces(config_filename) << std::endl;
		run_shell_command(command.str());
		return get_exe_return_status(config_filename);
	}

	ReturnCode convert_fluid_sim_to_hdf5(const std::string config_filename)
	{
		std::stringstream command;
		command << PARTICLE_TRACKING_EXECUTABLE_PATH << " --single-action convert_fluid_sim_to_hdf5 " << escape_spaces(config_filename) << std::endl;
		run_shell_command(command.str());
		return get_exe_return_status(config_filename);
	}


	ReturnCode extract_particle_mesh(const std::string config_filename)
	{
		std::stringstream command;
		command << PARTICLE_TRACKING_EXECUTABLE_PATH << " --single-action extract_particle_mesh " << escape_spaces(config_filename) << std::endl;
		run_shell_command(command.str());
		return get_exe_return_status(config_filename);
	}


	ReturnCode run_particle_tracking(const std::string config_filename)
	{
		std::stringstream command;
		command << PARTICLE_TRACKING_EXECUTABLE_PATH << " --single-action run_particle_tracking " << escape_spaces(config_filename) << std::endl;
		run_shell_command(command.str());
		return get_exe_return_status(config_filename);
	}

	ReturnCode emplace_particle_config_json(const std::string config_filename)
	{
		std::stringstream command;
		command << PARTICLE_TRACKING_EXECUTABLE_PATH << " --single-action emplace_particle_config_json " << escape_spaces(config_filename) << std::endl;
		run_shell_command(command.str());
		return get_exe_return_status(config_filename);
	}

	ReturnCode is_docker_server_running(const std::string config_filename)
	{
		std::stringstream command;
		command << PARTICLE_TRACKING_EXECUTABLE_PATH << " --single-action is_docker_server_running " << escape_spaces(config_filename) << std::endl;
		run_shell_command(command.str());
		return get_exe_return_status(config_filename);
	}

	ReturnCode finish(const std::string config_filename)
	{
		std::stringstream command;
		command << PARTICLE_TRACKING_EXECUTABLE_PATH << " --single-action finish " << escape_spaces(config_filename) << std::endl;
		run_shell_command(command.str());
		return get_exe_return_status(config_filename);
	}

	ReturnCode extract_final_data(const std::string config_filename)
	{
		std::stringstream command;
		command << PARTICLE_TRACKING_EXECUTABLE_PATH << " --single-action extract_final_data " << escape_spaces(config_filename) << std::endl;
		run_shell_command(command.str());
		return get_exe_return_status(config_filename);
	}

}
