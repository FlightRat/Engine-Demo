#include "UniformBuffer.h" 
#include <Logger/Logger.h>
#include <glad/glad.h> // 确保包含了支持 GL 4.5+ 核心的 glad 或 glew

namespace ENGINE_RENDERING {

    UniformBuffer::UniformBuffer(size_t size, int bindingPoint)
    {
        // 1. 直接创建底层缓冲对象 (DSA 核心)
        glCreateBuffers(1, &m_UboID);

        // 2. 分配不可变大小的显存块 (Immutable Storage)
        // 传入 nullptr 表示初始数据为空。
        // GL_DYNAMIC_STORAGE_BIT 标志非常关键！它告诉 GPU 驱动：
        // "这块内存的大小虽然锁死了，但我后续会高频使用 glNamedBufferSubData 从 CPU 向其写入数据，请优化访存策略！"
        glNamedBufferStorage(m_UboID, size, nullptr, GL_DYNAMIC_STORAGE_BIT);

        // 3. 将 Buffer 挂载到特定的 UBO 绑定点 (Binding Point)
        // 注意：glBindBufferRange 虽然名字里带 bind，但它是用于配置“管线绑定槽”的，是必须的。
        // 它并没有改变我们通过 ID 操作缓冲对象的过程。
        glBindBufferRange(GL_UNIFORM_BUFFER, bindingPoint, m_UboID, 0, size);
    }

    UniformBuffer::~UniformBuffer()
    {
        CleanUp();
    }

    void UniformBuffer::UpdateUniformBuffer(const void* data, size_t size, size_t offset)
    {
        // [现代引擎最精华的一行代码]
        // 没有任何 glBind，不需要恢复原状态，直接向目标 ID 所在的显存偏移处倾泻数据。
        // 这种调用不仅快，而且天生对多线程友好（不同线程更新不同 Buffer ID 极其安全）。
        glNamedBufferSubData(m_UboID, offset, size, data);
    }

    void UniformBuffer::CleanUp()
    {
        // 删除缓冲时最好检查一下 ID 是否有效，这是一个良好的引擎开发习惯
        if (m_UboID != 0) {
            glDeleteBuffers(1, &m_UboID);
            m_UboID = 0;
        }
    }
}