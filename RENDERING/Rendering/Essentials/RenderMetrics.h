#pragma once
#include <glad/glad.h>
#include <array>
#include <string>
#include <vector>

namespace ENGINE_RENDERING {

	struct LightRenderSample {
		std::array<double, 4> gpuMs{}; // geometry, culling, lighting, shadow
		std::array<double, 3> cpuMs{}; // collect, upload, submit
		double averageCandidates{ 0.0 }, weightedCandidates{ 0.0 }, weightedEvaluations{ 0.0 };
		unsigned int activeClusters{ 0 }, maxCandidates{ 0 }, overflowClusters{ 0 };
		bool statisticsAvailable{ false };
	};

	class RenderMetrics {
	private:
		struct PendingFrame {
			std::array<GLuint, 8> queries{};
			bool pending{ false }, culled{ false };
			unsigned int generation{ 0 };
			LightRenderSample sample;
		};
		struct PendingStats {
			GLuint buffer{ 0 };
			GLsync fence{ nullptr };
			size_t count{ 0 };
			unsigned int generation{ 0 }, lights{ 0 };
		};
		std::array<PendingFrame, 4> m_Frames{};
		std::array<PendingStats, 4> m_Stats{};
		int m_Frame{ 0 }, m_StatsFrame{ 0 }, m_Warmup{ 120 }, m_Target{ 300 };
		unsigned int m_Generation{ 0 };
		bool m_Recording{ false }, m_Active{ false };
		std::string m_Configuration;
		LightRenderSample m_Latest, m_Diagnostics;
		std::vector<LightRenderSample> m_Samples;
	public:
		~RenderMetrics();
		void Start(int warmup = 120, int samples = 300);
		void Configure(const std::string& configuration);
		void BeginFrame(bool culled);
		void BeginPass(int pass);
		void EndPass(int pass);
		void EndFrame(const std::array<double, 3>& cpuMs);
		void SampleCandidates(GLuint headers, size_t count, unsigned int lights);
		const auto& Samples() const { return m_Samples; }
		const auto& Latest() const { return m_Latest; }
		const auto& Configuration() const { return m_Configuration; }
		int WarmupRemaining() const { return m_Warmup; }
		bool IsRecording() const { return m_Recording; }
	};
}
