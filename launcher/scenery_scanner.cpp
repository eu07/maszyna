#include "stdafx.h"
#include "scenery_scanner.h"

void scenery_scanner::scan()
{
	for (auto &f : std::filesystem::directory_iterator("scenery")) {
		std::filesystem::path path(f.path());

		if (*(path.filename().string().begin()) == '$')
			continue;

		if (string_ends_with(path.string(), ".scn"))
			scan_scn(path.string());
	}

	std::sort(scenarios.begin(), scenarios.end(),
	          [](const scenery_desc &a, const scenery_desc &b) { return a.path < b.path; });
}

void scenery_scanner::scan_scn(std::string path)
{
	scenarios.emplace_back();
	scenery_desc &desc = scenarios.back();
	desc.path = path;

	std::ifstream stream(path, std::ios_base::binary | std::ios_base::in);

	std::string line;
	while (std::getline(stream, line))
	{
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
			desc.trainsets.emplace_back();
			trainset_desc &trainset = desc.trainsets.back();
			trainset.name = line.substr(5);
		}
	}
}
