#include "stdafx.h"
#include "widgets/trainingcard.h"
#include "simulation.h"

#ifdef __linux__
#include <unistd.h>
#include <sys/stat.h>
#endif

trainingcard_panel::trainingcard_panel()
    : ui_panel("Raport szkolenia", false)
{
	//size = {400, 500};
	clear();
}

trainingcard_panel::~trainingcard_panel()
{
	if (save_thread.joinable())
		save_thread.join();
}

void trainingcard_panel::clear()
{
	start_time_wall.reset();
	state.store(0);
	recording_timestamp.clear();

	place.clear();
	trainee_name.clear();
	trainee_birthdate.clear();
	trainee_company.clear();
	instructor_name.clear();
	track_segment.clear();
	remarks.clear();

	distance = 0.0f;

	place.resize(256, 0);
	trainee_name.resize(256, 0);
	trainee_birthdate.resize(256, 0);
	trainee_company.resize(256, 0);
	instructor_name.resize(256, 0);
	track_segment.resize(256, 0);
	remarks.resize(4096, 0);
}

void trainingcard_panel::save_thread_func()
{
	std::tm *tm = std::localtime(&(*start_time_wall));
	std::string date = std::to_string(tm->tm_year + 1900) + "-" + std::to_string(tm->tm_mon + 1) + "-" + std::to_string(tm->tm_mday);
	std::string from = std::to_string(tm->tm_hour) + ":" + std::to_string(tm->tm_min);
	std::time_t now = std::time(nullptr);
	tm = std::localtime(&now);
	std::string to = std::to_string(tm->tm_hour) + ":" + std::to_string(tm->tm_min);

	std::fstream temp("reports/" + recording_timestamp + ".html", std::ios_base::out | std::ios_base::binary);
	std::fstream input("report_template.html", std::ios_base::in | std::ios_base::binary);

	std::string in_line;
	while (std::getline(input, in_line)) {
		const std::string magic("{{CONTENT}}");
		if (in_line.compare(0, magic.size(), magic) == 0) {
			temp << "<div><b>Miejsce: </b>" << (std::string(place.c_str())) << "</div><br />" << std::endl;
			temp << "<div><b>Data: </b>" << (date) << "</div><br />" << std::endl;
			temp << "<div><b>Czas: </b>" << (from) << " - " << (to) << "</div><br />" << std::endl;
			temp << "<div><b>Imię (imiona) i nazwisko szkolonego: </b>" << (trainee_name) << "</div><br />" << std::endl;
			temp << "<div><b>Data urodzenia: </b>" << (trainee_birthdate) << "</div><br />" << std::endl;
			temp << "<div><b>Firma: </b>" << (trainee_company) << "</div><br />" << std::endl;
			temp << "<div><b>Imię i nazwisko instruktora: </b>"  << (instructor_name) << "</div><br />" << std::endl;
			temp << "<div><b>Odcinek trasy: </b>"  << (track_segment) << "</div><br />" << std::endl;
			if (distance > 0.0f)
				temp << "<div><b>Przebyta odległość: </b>"  << std::round(distance) << " km</div><br />" << std::endl;
			temp << "<div><b>Uwagi: </b><br />"  << (remarks) << "</div>" << std::endl;
		} else {
			temp << in_line;
		}
	}

	input.close();
	temp.close();

	state.store(EndRecording(recording_timestamp));
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

	ImGui::TextUnformatted("Data urodzenia:");
	ImGui::SameLine();
	ImGui::InputText("##birthdate", &trainee_birthdate[0], trainee_birthdate.size());

	ImGui::TextUnformatted("Firma:");
	ImGui::SameLine();
	ImGui::InputText("##company", &trainee_company[0], trainee_company.size());

	ImGui::TextUnformatted("Instruktor:");
	ImGui::SameLine();
	ImGui::InputText("##instructor", &instructor_name[0], instructor_name.size());

	ImGui::TextUnformatted("Odcinek trasy:");
	ImGui::SameLine();
	ImGui::InputText("##segment", &track_segment[0], track_segment.size());

	ImGui::TextUnformatted("Uwagi");
	ImGui::InputTextMultiline("##remarks", &remarks[0], remarks.size(), ImVec2(-1.0f, 200.0f));

	if (!start_time_wall) {
		if (ImGui::Button("Rozpocznij szkolenie")) {
			start_time_wall = std::time(nullptr);
			std::tm *tm = std::localtime(&(*start_time_wall));
			recording_timestamp = std::to_string(tm->tm_year + 1900) + std::to_string(tm->tm_mon + 1) + std::to_string(tm->tm_mday)
			    + std::to_string(tm->tm_hour) + std::to_string(tm->tm_min) + "_" + std::string(trainee_name.c_str()) + "_" + std::string(instructor_name.c_str());

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
			if (simulation::Trains.sequence().size() > 0)
				distance = simulation::Trains.sequence()[0]->Dynamic()->MoverParameters->DistCounter;

			if (save_thread.joinable())
				save_thread.join();
			save_thread = std::thread(&trainingcard_panel::save_thread_func, this);
			ImGui::OpenPopup("Zapisywanie danych");
		}
	}
}

const std::string *trainingcard_panel::is_recording()
{
	if (!start_time_wall)
		return nullptr;

	return &recording_timestamp;
}
