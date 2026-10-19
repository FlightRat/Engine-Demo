#include "RenderMetrics.h"
#include <algorithm>
#include <cstdint>

namespace ENGINE_RENDERING {

	RenderMetrics::~RenderMetrics()
	{
		for (auto& frame : m_Frames) glDeleteQueries(8, frame.queries.data());
		for (auto& stats : m_Stats) {
			if (stats.fence) glDeleteSync(stats.fence);
			glDeleteBuffers(1, &stats.buffer);
		}
	}

	void RenderMetrics::Start(int warmup, int samples)
	{
		++m_Generation;
		m_Samples.clear();
		m_Warmup = warmup;
		m_Target = samples;
		m_Recording = true;
		m_Latest = m_Diagnostics = {};
	}

	void RenderMetrics::Configure(const std::string& configuration)
	{
		if (configuration == m_Configuration) return;
		m_Configuration = configuration;
		Start();
	}

	void RenderMetrics::BeginFrame(bool culled)
	{
		for (auto& frame : m_Frames) {
			if (!frame.pending) continue;
			GLint ready;
			glGetQueryObjectiv(frame.queries[5], GL_QUERY_RESULT_AVAILABLE, &ready);
			if (!ready) continue;
			frame.pending = false;
			if (frame.generation != m_Generation) continue;
			for (int pass = 0; pass < 4; ++pass) {
				GLuint64 begin, end;
				glGetQueryObjectui64v(frame.queries[pass * 2], GL_QUERY_RESULT, &begin);
				glGetQueryObjectui64v(frame.queries[pass * 2 + 1], GL_QUERY_RESULT, &end);
				frame.sample.gpuMs[pass] = (end - begin) / 1e6;
			}
			if (!frame.culled) frame.sample.gpuMs[1] = 0.0;
			m_Latest = frame.sample;
			if (m_Recording) {
				if (m_Warmup > 0) --m_Warmup;
				else m_Samples.push_back(frame.sample);
				if (m_Samples.size() == static_cast<size_t>(m_Target)) m_Recording = false;
			}
		}
		auto& frame = m_Frames[m_Frame];
		m_Active = !frame.pending;
		if (!m_Active) return;
		if (frame.queries[0] == 0) glGenQueries(8, frame.queries.data());
		frame.generation = m_Generation;
		frame.culled = culled;
		frame.sample = {};
	}

	void RenderMetrics::BeginPass(int pass)
	{
		if (m_Active) glQueryCounter(m_Frames[m_Frame].queries[pass * 2], GL_TIMESTAMP);
	}

	void RenderMetrics::EndPass(int pass)
	{
		if (m_Active) glQueryCounter(m_Frames[m_Frame].queries[pass * 2 + 1], GL_TIMESTAMP);
	}

	void RenderMetrics::EndFrame(const std::array<double, 3>& cpuMs)
	{
		if (m_Active) {
			auto& frame = m_Frames[m_Frame];
			frame.sample = m_Diagnostics;
			frame.sample.cpuMs = cpuMs;
			frame.pending = true;
		}
		m_Frame = (m_Frame + 1) % static_cast<int>(m_Frames.size());
	}

	void RenderMetrics::SampleCandidates(GLuint headers, size_t count, unsigned int lights)
	{
		for (auto& stats : m_Stats) {
			if (!stats.fence) continue;
			GLenum status = glClientWaitSync(stats.fence, 0, 0);
			if (status != GL_ALREADY_SIGNALED && status != GL_CONDITION_SATISFIED) continue;
			glDeleteSync(stats.fence);
			stats.fence = nullptr;
			if (stats.generation != m_Generation) continue;
			std::vector<std::array<unsigned int, 4>> values(stats.count);
			glGetNamedBufferSubData(stats.buffer, 0, values.size() * sizeof(values[0]), values.data());
			m_Diagnostics = {};
			std::uint64_t sum = 0, weighted = 0, evaluated = 0, pixels = 0;
			for (auto value : values) {
				if (value[2] == 0) continue;
				++m_Diagnostics.activeClusters;
				sum += value[0];
				pixels += value[2];
				weighted += std::uint64_t(value[0]) * value[2];
				evaluated += std::uint64_t(value[1] ? stats.lights : value[0]) * value[2];
				m_Diagnostics.maxCandidates = (std::max)(m_Diagnostics.maxCandidates, value[0]);
				m_Diagnostics.overflowClusters += value[1];
			}
			if (m_Diagnostics.activeClusters) m_Diagnostics.averageCandidates = double(sum) / m_Diagnostics.activeClusters;
			if (pixels) {
				m_Diagnostics.weightedCandidates = double(weighted) / pixels;
				m_Diagnostics.weightedEvaluations = double(evaluated) / pixels;
			}
			m_Diagnostics.statisticsAvailable = true;
			m_Diagnostics.gpuMs = m_Latest.gpuMs;
			m_Diagnostics.cpuMs = m_Latest.cpuMs;
			m_Latest = m_Diagnostics;
		}
		auto& stats = m_Stats[m_StatsFrame];
		if (stats.fence) return;
		if (stats.buffer == 0) glCreateBuffers(1, &stats.buffer);
		stats.count = count;
		stats.lights = lights;
		stats.generation = m_Generation;
		glNamedBufferData(stats.buffer, count * 16, nullptr, GL_STREAM_READ);
		glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
		glCopyNamedBufferSubData(headers, stats.buffer, 0, 0, count * 16);
		stats.fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
		m_StatsFrame = (m_StatsFrame + 1) % static_cast<int>(m_Stats.size());
	}
}
