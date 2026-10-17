#include "RenderMetrics.h"
#include <algorithm>

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
		m_Samples.clear();
		m_Warmup = warmup;
		m_Target = samples;
		m_Recording = true;
	}

	void RenderMetrics::BeginFrame(bool tiled)
	{
		for (auto& frame : m_Frames) {
			if (!frame.pending) continue;
			GLint ready;
			glGetQueryObjectiv(frame.queries[5], GL_QUERY_RESULT_AVAILABLE, &ready);
			if (!ready) continue;
			for (int pass = 0; pass < 4; ++pass) {
				GLuint64 begin, end;
				glGetQueryObjectui64v(frame.queries[pass * 2], GL_QUERY_RESULT, &begin);
				glGetQueryObjectui64v(frame.queries[pass * 2 + 1], GL_QUERY_RESULT, &end);
				frame.sample.gpuMs[pass] = (end - begin) / 1e6;
			}
			if (!frame.tiled) frame.sample.gpuMs[1] = 0.0;
			m_Latest = frame.sample;
			frame.pending = false;
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
		frame.tiled = tiled;
		frame.sample = m_Latest;
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
			frame.sample.cpuMs = cpuMs;
			frame.sample.averageCandidates = m_Latest.averageCandidates;
			frame.sample.maxCandidates = m_Latest.maxCandidates;
			frame.sample.overflowTiles = m_Latest.overflowTiles;
			frame.pending = true;
		}
		m_Frame = (m_Frame + 1) % static_cast<int>(m_Frames.size());
	}

	void RenderMetrics::SampleCandidates(GLuint headers, size_t tiles)
	{
		for (auto& stats : m_Stats) {
			if (!stats.fence) continue;
			GLenum status = glClientWaitSync(stats.fence, 0, 0);
			if (status != GL_ALREADY_SIGNALED && status != GL_CONDITION_SATISFIED) continue;
			std::vector<std::array<unsigned int, 2>> values(stats.bytes / 8);
			glGetNamedBufferSubData(stats.buffer, 0, stats.bytes, values.data());
			unsigned long long sum = 0;
			m_Latest.maxCandidates = m_Latest.overflowTiles = 0;
			for (auto value : values) {
				sum += value[0];
				m_Latest.maxCandidates = std::max(m_Latest.maxCandidates, value[0]);
				m_Latest.overflowTiles += value[1];
			}
			m_Latest.averageCandidates = static_cast<double>(sum) / values.size();
			glDeleteSync(stats.fence);
			stats.fence = nullptr;
		}
		auto& stats = m_Stats[m_StatsFrame];
		if (stats.fence) return;
		if (stats.buffer == 0) glCreateBuffers(1, &stats.buffer);
		stats.bytes = tiles * 8;
		glNamedBufferData(stats.buffer, stats.bytes, nullptr, GL_STREAM_READ);
		glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
		glCopyNamedBufferSubData(headers, stats.buffer, 0, 0, stats.bytes);
		stats.fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
		m_StatsFrame = (m_StatsFrame + 1) % static_cast<int>(m_Stats.size());
	}
}
