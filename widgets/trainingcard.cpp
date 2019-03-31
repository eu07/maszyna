#include "stdafx.h"
#include "widgets/trainingcard.h"

trainingcard_panel::trainingcard_panel()
    : ui_panel("Raport szkolenia", false)
{
	//size = {400, 500};
	clear();
}

void trainingcard_panel::clear()
{
	start_time_wall.reset();
	state.store(0);

	place.clear();
	trainee_name.clear();
	instructor_name.clear();
	remarks.clear();

	place.resize(256, 0);
	trainee_name.resize(256, 0);
	instructor_name.resize(256, 0);
	remarks.resize(4096, 0);
}

void trainingcard_panel::save_thread_func()
{
	std::tm *tm = std::localtime(&(*start_time_wall));
	std::string rep = std::to_string(tm->tm_year + 1900) + std::to_string(tm->tm_mon + 1) + std::to_string(tm->tm_mday)
	        + std::to_string(tm->tm_hour) + std::to_string(tm->tm_min) + "_" + trainee_name + "_" + instructor_name;
	state.store(EndRecording(rep));
}

void trainingcard_panel::render_contents()
{
	if (ImGui::BeginPopupModal("Zapisywanie danych")) {
		ImGui::SetWindowSize(ImVec2(-1, -1));
		int s = state.load();

		if (s == 1) {
			ImGui::CloseCurrentPopup();
			clear();
		}

		if (s < 1) {
			if (s == 0)
				ImGui::TextUnformatted("Error occured please contact with administrator!");
			if (s == -1)
				ImGui::TextUnformatted("The recording of the training has not been archived, please do not start the next training before manually archiving the file!");

			if (ImGui::Button("OK")) {
				ImGui::CloseCurrentPopup();
				clear();
			}
		}

		if (s == 2)
			ImGui::TextUnformatted(u8"Proszę czekać, trwa archiwizacja nagrania...");

		ImGui::EndPopup();
	}

	if (start_time_wall) {
		std::tm *tm = std::localtime(&(*start_time_wall));
		std::string rep = u8"Czas rozpoczęcia: " + std::to_string(tm->tm_year + 1900) + "-" + std::to_string(tm->tm_mon + 1) + "-" + std::to_string(tm->tm_mday)
		        + " " + std::to_string(tm->tm_hour) + ":" + std::to_string(tm->tm_min);
		ImGui::TextUnformatted(rep.c_str());
	}

	ImGui::TextUnformatted("Miejsce:");
	ImGui::SameLine();
	ImGui::InputText("##place", &place[0], place.size());

	ImGui::TextUnformatted("Szkolony:");
	ImGui::SameLine();
	ImGui::InputText("##trainee", &trainee_name[0], trainee_name.size());

	ImGui::TextUnformatted("Instruktor:");
	ImGui::SameLine();
	ImGui::InputText("##instructor", &instructor_name[0], instructor_name.size());

	ImGui::TextUnformatted("Uwagi");
	ImGui::InputTextMultiline("##remarks", &remarks[0], remarks.size(), ImVec2(-1.0f, 200.0f));

	if (!start_time_wall) {
		if (ImGui::Button("Rozpocznij szkolenie")) {
			start_time_wall = std::time(nullptr);

			int ret = StartRecording();
			if (ret != 1) {
				state.store(ret);
				ImGui::OpenPopup("Zapisywanie danych");
			}
		}
	}
	else {
		if (ImGui::Button(u8"Zakończ szkolenie")) {
			state.store(2);
			if (save_thread.joinable())
				save_thread.join();
			save_thread = std::thread(&trainingcard_panel::save_thread_func, this);
			ImGui::OpenPopup("Zapisywanie danych");
		}
	}
}
