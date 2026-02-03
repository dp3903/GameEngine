#include "egpch.h"
#include "Project.h"

#include "ProjectSerializer.h"

namespace Engine {

	std::shared_ptr<Project> Project::New(const std::filesystem::path& projectDir)
	{
		s_ActiveProject = std::make_shared<Project>();
		s_ActiveProject->m_ProjectDirectory = projectDir;

		ProjectSerializer serializer(s_ActiveProject);
		serializer.Serialize(projectDir / (s_ActiveProject->GetConfig().Name + ".gmproj"));
		
		return s_ActiveProject;
	}

	std::shared_ptr<Project> Project::Load(const std::filesystem::path& path)
	{
		std::shared_ptr<Project> project = std::make_shared<Project>();

		ProjectSerializer serializer(project);
		if (serializer.Deserialize(path))
		{
			project->m_ProjectDirectory = path.parent_path();
			s_ActiveProject = project;
			return s_ActiveProject;
		}

		return nullptr;
	}

	bool Project::SaveActive()
	{
		ProjectSerializer serializer(s_ActiveProject);
		if (serializer.Serialize(s_ActiveProject->m_ProjectDirectory / (s_ActiveProject->GetConfig().Name + ".gmproj")))
		{
			return true;
		}

		return false;
	}

}