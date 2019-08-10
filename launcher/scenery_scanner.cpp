#include "stdafx.h"
#include "scenery_scanner.h"
#include "Logs.h"

void scenery_scanner::scan()
{
	for (auto &f : std::filesystem::directory_iterator("scenery")) {
		std::filesystem::path path(f.path());

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

	cParser parser(path.string(), cParser::buffer_FILE);
	parser.expandIncludes = false;
	while (!parser.eof()) {
		parser.getTokens();
		if (parser.peek() == "trainset")
			parse_trainset(parser);
	}

	std::ifstream stream(path.string(), std::ios_base::binary | std::ios_base::in);

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
			desc.name = line.substr(5);
		else if (line[3] == 'd')
			desc.description += line.substr(5) + '\n';
		else if (line[3] == 'f') {
			std::string lang;
			std::string file;
			std::string label;

			std::istringstream stream(line.substr(5));
			stream >> lang >> file;
			std::getline(stream, label);

			desc.links.push_back(std::make_pair(file, label));
		}
		else if (line[3] == 'o') {
			for (trainset_desc &trainset : desc.trainsets) {
				if (line_counter < trainset.file_bounds.first
				        || line_counter > trainset.file_bounds.second)
					continue;

				trainset.description = line.substr(5);
			}
		}
	}

	WriteLog(path.string() + "----------");
	for (trainset_desc &trainset : desc.trainsets) {
		WriteLog(trainset.name + "=" + std::to_string(trainset.file_bounds.first) + ":" +  std::to_string(trainset.file_bounds.second) + "@" + trainset.description);
	}
}

void scenery_scanner::parse_trainset(cParser &parser)
{
	scenery_desc &desc = scenarios.back();
	desc.trainsets.emplace_back();
	trainset_desc &trainset = desc.trainsets.back();

	trainset.file_bounds.first = parser.Line();
	parser.getTokens(4);
	parser >> trainset.name >> trainset.track >> trainset.offset >> trainset.velocity;

	parser.getTokens();
	while (parser.peek() == "node") {
		trainset.vehicles.emplace_back();
		dynamic_desc &dyn = trainset.vehicles.back();

		std::string datafolder, skinfile, mmdfile;
		int offset;

		parser.getTokens(2); // range_max, range_min
		parser.getTokens(1, false); // name
		parser >> dyn.name;

		if (!parser.expectToken("dynamic"))
			break;

		parser.getTokens(7, false);
		parser >> datafolder >> skinfile >> mmdfile >> offset >> dyn.drivertype >> dyn.coupling >> dyn.loadcount;

		dyn.skin = datafolder + "/" + skinfile;
		dyn.mmd = datafolder + "/" + mmdfile;

		parser.getTokens();
		if (parser.peek() != "enddynamic") {
			parser >> dyn.loadtype;
			parser.getTokens();
		}

		parser.getTokens();
	}

	trainset.file_bounds.second = parser.Line();
}
