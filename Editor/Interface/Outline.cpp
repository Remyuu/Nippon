#include <Editor/Ecs/Registry.h>
#include <Editor/Ecs/Entity.h>

#include <Editor/Ecs/Components/Camera.h>
#include <Editor/Ecs/Components/Transform.h>

#include <Editor/Export/WavefrontExporter.h>

#include <Editor/Font/MaterialDesignIcons.h>

#include <Editor/Interface/Outline.h>

#include <Editor/ImGui/imgui.h>

#include <Editor/Scene/Scene.h>
#include <Editor/Scene/SceneManager.h>

namespace Nippon
{
	static Entity* sSelectedEntity = nullptr;
	static bool sFocusPlayer = false;
	static R32V3 sFocusPlayerPosition = {};

	void Outline::Reset()
	{
		sSelectedEntity = nullptr;
		sFocusPlayer = false;
	}

	void Outline::Render()
	{
		ImGui::Begin(ICON_MDI_FILE_TREE " Outline");

		if (Scene* scene = SceneManager::GetCurrentScene())
		{
			if (Registry* registry = scene->GetRegistry())
			{
				if (sFocusPlayer)
				{
					if (Transform* playerTransform = registry->GetPlayerEntity()->GetTransform())
					{
						R32V3 delta = sFocusPlayerPosition - playerTransform->GetWorldPosition();
						R32 alpha = glm::min(ImGui::GetIO().DeltaTime * 8.0F, 1.0F);

						playerTransform->SetPosition(playerTransform->GetLocalPosition() + delta * alpha);
						scene->Invalidate();

						sFocusPlayer = glm::length(delta) > 0.01F;
					}
				}

				if (Entity* entity = registry->GetRootEntity())

				DrawEntityTreeRecursive(scene, entity);
			}
		}

		ImGui::End();
	}

	Entity* Outline::GetSelectedEntity()
	{
		return sSelectedEntity;
	}

	void Outline::CancelFocus()
	{
		sFocusPlayer = false;
	}

	void Outline::SetSelectedEntity(Entity* Entity)
	{
		if (Entity)
		{
			Entity->SetOpenedUp(true);
		}

		sSelectedEntity = Entity;
	}

	bool Outline::DrawEntityTreeRecursive(Scene* Scene, Entity* Entity)
	{
		bool dirty = false;

		ImGui::PushID(Entity);

		U32 flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding;

		if (Entity == sSelectedEntity) flags |= ImGuiTreeNodeFlags_Selected;
		if (Entity->IsOpened())        flags |= ImGuiTreeNodeFlags_DefaultOpen;
		if (Entity->IsChild())         flags |= ImGuiTreeNodeFlags_Leaf;

		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 5.0F, 5.0F });

		if (Entity->IsActive()) ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_Text]);
		else                    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);

		U32 opened = ImGui::TreeNodeEx(Entity->GetName().data(), flags);

		if (ImGui::IsItemClicked(0) || ImGui::IsItemClicked(1))
		{
			sSelectedEntity = Entity;

			Scene->Invalidate();
		}

		if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
		{
			if (Registry* registry = Scene->GetRegistry())
			{
				if (Transform* playerTransform = registry->GetPlayerEntity()->GetTransform())
				{
					R32V3 center = Entity->GetTransform()->GetWorldPosition() + Entity->GetAABB().GetCenter();
					R32 radius = glm::length(Entity->GetAABB().GetSize()) * 0.5F;
					R32 distance = glm::max(radius / glm::tan(glm::radians(registry->GetMainCamera()->GetFov()) * 0.5F) * 1.25F, 10.0F);

					sFocusPlayerPosition = center + playerTransform->GetLocalFront() * distance;
					sFocusPlayer = true;
				}
			}
		}

		if (ImGui::BeginPopupContextItem("Outline Context Menu"))
		{
			if (ImGui::MenuItem(ICON_MDI_PLUS " Add"))
			{
				if (Registry* registry = Scene->GetRegistry())
				{
					registry->CreateEntity("", sSelectedEntity);

					dirty = true;
				}
			}

			if (ImGui::MenuItem(ICON_MDI_MINUS " Remove"))
			{
				if (sSelectedEntity)
				{
					sSelectedEntity->SetShouldBeDestroyed();

					Scene->Invalidate();

					dirty = true;
				}
			}

			if (ImGui::MenuItem(ICON_MDI_MINUS " Remove Recursive"))
			{
				if (sSelectedEntity)
				{
					sSelectedEntity->SetShouldBeDestroyedDown();

					Scene->Invalidate();

					dirty = true;
				}
			}

			ImGui::Separator();

			if (ImGui::MenuItem(ICON_MDI_RENAME " Rename"))
			{
				// TODO
			}

			ImGui::Separator();

			if (ImGui::MenuItem(ICON_MDI_EXPORT " Export as Wavefront"))
			{
				if (sSelectedEntity)
				{
					WavefrontExporter::Export(sSelectedEntity, Scene);
				}
			}

			ImGui::EndPopup();
		}

		ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - 25.0F);
		if (ImGui::Button(ICON_MDI_CLOSE_THICK, ImVec2{ 25.0F, 0.0F }))
		{
			Entity->SetShouldBeDestroyedDown();

			Scene->Invalidate();
			
		}
		ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - 50.0F);
		if (ImGui::Button(Entity->IsActive() ? ICON_MDI_EYE_OUTLINE : ICON_MDI_EYE_OFF_OUTLINE, ImVec2{ 25.0F, 0.0F }))
		{
			Entity->SetActiveDown(!Entity->IsActive());

			Scene->Invalidate();
		}

		ImGui::PopStyleColor();

		if (opened)
		{
			if (!dirty)
			{
				for (auto& child : Entity->GetChildren())
				{
					dirty = DrawEntityTreeRecursive(Scene, child);

					if (dirty)
					{
						break;
					}
				}
			}

			ImGui::TreePop();
		}
		else
		{
			//Entity->SetOpenedDown(false); // TODO
		}

		ImGui::PopStyleVar();

		ImGui::PopID();

		return dirty;
	}
}
