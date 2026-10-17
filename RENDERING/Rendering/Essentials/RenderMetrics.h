#pragma once
#include <glad/glad.h>
#include <array>
#include <vector>
#include <cstddef>

namespace ENGINE_RENDERING {
	struct LightRenderSample {
		std::array<double, 4> gpuMs{}; // geometry, culling, lighting, shadow
		std::array<double, 3> cpuMs{}; // collect, upload, submit
		double averageCandidates{ 0.0 };
		unsigned int maxCandidates{ 0 }, overflowTiles{ 0 };
	};

	class RenderMetrics
	{
	private:
		struct PendingFrame {
			std::array<GLuint, 8> queries{};
			bool pending{ false }, tiled{ false };
			LightRenderSample sample;
		};
		struct PendingStats {
			GLuint buffer{ 0 };
			GLsync fence{ nullptr };
			size_t bytes{ 0 };
		};
		std::array<PendingFrame, 4> m_Frames{};
		std::array<PendingStats, 4> m_Stats{};
		int m_Frame{ 0 }, m_StatsFrame{ 0 }, m_Warmup{ 120 }, m_Target{ 300 };
		bool m_Recording{ false }, m_Active{ false };
		LightRenderSample m_Latest;
		std::vector<LightRenderSample> m_Samples;
	public:
		~RenderMetrics();
		void Start(int warmup = 120, int samples = 300);
		void BeginFrame(bool tiled);
		void BeginPass(int pass);
		void EndPass(int pass);
		void EndFrame(const std::array<double, 3>& cpuMs);
		void SampleCandidates(GLuint headers, size_t tiles);
		inline const std::vector<LightRenderSample>& Samples() const { return m_Samples; }
		inline const LightRenderSample& Latest() const { return m_Latest; }
	};
}
