// Markus Buchholz, 2023

#include <stdafx.hpp>
#include "imgui_helper.hpp"
#include <tuple>
#include <vector>
#include <math.h>
#include <random>
#include <algorithm>
#include <thread>
#include <chrono>
#include <list>

//---------------------------------------------------------------

int W = 800;
int H = 800;

float dt = 5.0f;

ImVec2 C(400.0f, 400.0f);
float step = 20.0f;
int scale = 5;
int mesh_size = 80;
float sim_offset = 150.0f;

//---------------------------------------------------------------

std::vector<float> flip(std::vector<float> v)
{
	std::vector<float> flipped(v.size(), 0.0f);

	for (int ii = 0; ii < v.size(); ii++)
	{
		flipped[ii] = v[v.size() - 1 - ii];
	}

	return flipped;
};

//---------------------------------------------------------------
// this function is not used in this code but I would like to keep
std::vector<float> shift(std::vector<float> v, int N)
{
	std::vector<float> zeros(N, 0.0f);

	std::vector<float> shifted(v);
	shifted.insert(shifted.end(), zeros.begin(), zeros.end());
	return shifted;
};

//---------------------------------------------------------------
// this function is not used in this code but I would like to keep
std::vector<float> conv1D(std::vector<float> x, std::vector<float> h)
{
	auto N = x.size();
	auto h1 = flip(h);
	auto h2 = shift(h1, N);

	std::vector<float> y(x.size() + h.size() - 1, 0);

	for (int ii = 0; ii < h.size(); ii++)
	{

		for (int jj = 0; jj < x.size(); jj++)
		{

			y[ii + jj] = y[ii + jj] + h[ii] * x[jj];
		}
	}

	return y;
}

//---------------------------------------------------------------
int main(int argc, char const *argv[])
{

	std::string title = "Marching Squares";
	initImgui(W, H, title);

	bool show_demo_window = true;
	bool show_another_window = false;
	ImVec4 clear_color = ImVec4(0.0f / 255.0, 0.0f / 255.0, 0.0f / 255.0, 1.00f);
	ImVec4 white_color = ImVec4(255.0f / 255.0, 255.0f / 255.0, 255.0f / 255.0, 1.00f);
	ImVec4 blue_color = ImVec4(0.0f / 255.0, 0.0f / 255.0, 170.0f / 255.0, 1.00f);
	ImVec4 pink_color = ImVec4(179.0f / 255.0, 12.0f / 255.0, 130.0f / 255.0, 1.00f);
	ImVec4 gray_color = ImVec4(150.0f / 255.0, 10.0f / 160.0, 170.0f / 255.0, 1.00f);
	ImVec4 black_color = ImVec4(0.0f / 255.0, 0.0f / 255.0, 0.0f / 255.0, 1.00f);

	bool flag = true;

	std::vector<float> x = {8, 7, 6, 5, 4, 3, 2, 1, 0};
	std::vector<float> h = {5, 5, 5, 5, 5, 5, 5, 5, 5};

	std::vector<float> sigX(mesh_size, 0);
	std::vector<float> sig_h_disp_vec(mesh_size, 0);
	std::vector<float> sigH_vec(mesh_size, 0);
	std::vector<float> sig_xh(mesh_size, 0);

	int offset_x = 20;
	for (int ii = 0; ii < x.size(); ii++)
	{
		sigX[ii + offset_x] = x[ii];
	}

	int offset_h = 0;
	for (int ii = 0; ii < h.size(); ii++)
	{
		sig_h_disp_vec[ii + offset_h] = h[ii];
	}

	auto h_flip = flip(h);

	for (int ii = 0; ii < h_flip.size(); ii++)
	{
		sigH_vec[ii + offset_h] = h_flip[ii];
	}

	std::list<float> sigH(sigH_vec.begin(), sigH_vec.end());
	std::list<float> sig_h_disp(sig_h_disp_vec.begin(), sig_h_disp_vec.end());

	int index = 0;
	while (!glfwWindowShouldClose(window) && flag == true)
	{
		glfwPollEvents();

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();

		ImGui::NewFrame();

		ImGuiWindowFlags window_flags = 0;
		ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(ImVec2(W, H), ImGuiCond_FirstUseEver);
		window_flags |= ImGuiWindowFlags_NoMove;
		window_flags |= ImGuiWindowFlags_NoResize;
		window_flags |= ImGuiWindowFlags_NoCollapse;
		window_flags |= ImGuiWindowFlags_NoBackground;
		window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus;

		ImGui::Begin("Simulation", nullptr, window_flags);
		ImDrawList *draw_list = ImGui::GetWindowDrawList();

		// draw mesh

		for (int ii = 0; ii < mesh_size; ii++)
		{
			for (int jj = 0; jj < mesh_size; jj++)
			{
				draw_list->AddCircleFilled({ii * step, jj * step}, 0.3f, ImColor(white_color));
			}
		}


		for (int ii = 0; ii < sigX.size(); ii++)
		{
			if (sigX[ii] != 0)
			{

				draw_list->AddCircleFilled({ii * step, C.y - sigX[ii] * step}, 4.0f, ImColor(blue_color));
				draw_list->AddLine({ii * step, C.y}, {ii * step, C.y - sigX[ii] * step}, ImColor(blue_color), 0.1f);
			}
		}

		int ix = 1;
		for (auto &ii : sig_h_disp)
		{
			if (ii != 0)
			{

				draw_list->AddCircleFilled({ix * step, C.y - ii * step}, 4.0f, ImColor(pink_color));
				draw_list->AddLine({ix * step, C.y}, {ix * step, C.y - ii * step}, ImColor(pink_color), 0.1f);
			}
			ix++;
		}

		sig_h_disp.pop_back();
		sig_h_disp.push_front(0);
		sigH.pop_back();
		sigH.push_front(0);

		int sumXH = 0;

		int jj = 0;

		for (auto &ii : sigH)
		{
			sumXH += sigX[jj] * ii;
			jj++;
		}

		if (sig_xh[index] == 0)
		{

			sig_xh[index] = sumXH;
		}
		index++;

		for (int ii = 0; ii < sig_xh.size(); ii++)
		{
			if (sig_xh[ii] != 0)
			{
				draw_list->AddCircleFilled({ii * step, (float)H - 200.0f - sig_xh[ii]}, 4.0f, ImColor(white_color));
				draw_list->AddLine({ii * step, (float)H - 200.0f}, {ii * step, (float)H - 200.0f - sig_xh[ii]}, ImColor(white_color), 0.1f);
			}
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(200));

		ImGui::End();

		// Rendering
		ImGui::Render();
		int display_w, display_h;
		glfwGetFramebufferSize(window, &display_w, &display_h);
		glViewport(0, 0, display_w, display_h);
		glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
		glClear(GL_COLOR_BUFFER_BIT);
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(window);
	}

	termImgui();
	return 0;
}
