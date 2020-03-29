#include "stdafx.h"
#include "scenery_scanner.h"
#include "Logs.h"

scenery_scanner::scenery_scanner(ui::vehicles_bank &bank)
    : bank(bank)
{

}

void scenery_scanner::scan()
{
	for (auto &f : std::filesystem::directory_iterator("scenery")) {
		std::filesystem::path path(std::filesystem::relative(f.path(), "scenery/"));

		if (*(path.filename().string().begin()) == '$')
			continue;

		if (string_ends_with(path.string(), ".scn"))
			scan_scn(path);
	}

	std::sort(scenarios.begin(), scenarios.end(),
	          [](const scenery_desc &a, const scenery_desc &b) { return a.path < b.path; });
}

void scenery_scanner::scan_scn(std::filesystem::path path)
{
	scenarios.emplace_back();
	scenery_desc &desc = scenarios.back();
	desc.path = path;

	std::string file_path = "scenery/" + path.string();

	cParser parser(file_path, cParser::buffer_FILE);
	parser.expandIncludes = false;
	while (!parser.eof()) {
		parser.getTokens();
		if (parser.peek() == "trainset")
			parse_trainset(parser);
	}

	std::ifstream stream(file_path, std::ios_base::binary | std::ios_base::in);

	int line_counter = 0;
	std::string line;
	while (std::getline(stream, line))
	{
		line_counter++;

		if (line.size() < 6 || !string_starts_with(line, "//$"))
			continue;

		if (line[3] == 'i')
			desc.image_path = "scenery/images/" + line.substr(5);
		else if (line[3] == 'n')
			desc.name = win1250_to_utf8(line.substr(5));
		else if (line[3] == 'd')
			desc.description += win1250_to_utf8(line.substr(5)) + '\n';
		else if (line[3] == 'f') {
			std::string lang;
			std::string file;
			std::string label;

			std::istringstream stream(line.substr(5));
			stream >> lang >> file;
			std::getline(stream, label);

			desc.links.push_back(std::make_pair(file, win1250_to_utf8(label)));
		}
		else if (line[3] == 'o') {
			for (auto &trainset : desc.trainsets) {
				if (line_counter < trainset.file_bounds.first
				        || line_counter > trainset.file_bounds.second)
					continue;

				trainset.description = win1250_to_utf8(line.substr(5));
			}
		}
	}
}

void scenery_scanner::parse_trainset(cParser &parser)
{
	scenery_desc &desc = scenarios.back();
	desc.trainsets.emplace_back();
	auto &trainset = desc.trainsets.back();

	trainset.file_bounds.first = parser.Line();
	parser.getTokens(4);
	parser >> trainset.name >> trainset.track >> trainset.offset >> trainset.velocity;

	parser.getTokens();
	while (parser.peek() == "node") {
		trainset.vehicles.emplace_back();
		dynamic_desc &dyn = trainset.vehicles.back();

		std::string datafolder, skinfile, mmdfile, params;

		parser.getTokens(2); // range_max, range_min
		parser.getTokens(1, false); // name
		parser >> dyn.name;

		if (!parser.expectToken("dynamic"))
			break;

		parser.getTokens(7, false);
		parser >> datafolder >> skinfile >> mmdfile >> dyn.offset >> dyn.drivertype >> params >> dyn.loadcount;

		size_t params_pos = params.find('.');
		if (params_pos != -1 && params_pos < params.size()) {
			dyn.params = params.substr(params_pos + 1, -1);
			params.erase(params_pos);
		}

		{
			std::istringstream couplingparser(params);
			couplingparser >> dyn.coupling;
		}

		skinfile = ToLower(skinfile);
		erase_extension(skinfile);
		replace_slashes(datafolder);
		auto vehicle_path = std::filesystem::path(ToLower(datafolder)) / ToLower(mmdfile);

		auto it = bank.vehicles.find(vehicle_path);
		if (it != bank.vehicles.end()) {
			dyn.vehicle = it->second;
			bool found = false;

			for (const std::shared_ptr<ui::skin_set> &vehicle_skin : it->second->matching_skinsets) {
				if (vehicle_skin->skin != skinfile)
					continue;

				dyn.skin = vehicle_skin;
				found = true;
				break;
			}
			if (!found && skinfile != "none")
				ErrorLog("skin not found: " + skinfile + ", vehicle type: " + vehicle_path.string() + ", file: " + parser.Name(), logtype::file);
		}
		else {
			ErrorLog("vehicle type not found: " + vehicle_path.string() + ", file: " + parser.Name(), logtype::file);
		}

		parser.getTokens();
		if (parser.peek() != "enddynamic") {
			parser >> dyn.loadtype;
			parser.getTokens();
		}

		parser.getTokens();
	}

	trainset.file_bounds.second = parser.Line();
}
