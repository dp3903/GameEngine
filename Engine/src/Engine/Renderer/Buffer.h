#pragma once
#include <glad/glad.h>

namespace Engine {

	
	class ShaderDataType
	{
		struct ShaderDataTypeProps
		{
			GLenum type;
			GLuint size;
			GLuint count;
		};
	public:
		static constexpr ShaderDataTypeProps None		= { GL_NONE,   0,					 0 };
		static constexpr ShaderDataTypeProps Float		= { GL_FLOAT,  1 * sizeof(GLfloat),  1 };
		static constexpr ShaderDataTypeProps Float2		= { GL_FLOAT,  2 * sizeof(GLfloat),  2 };
		static constexpr ShaderDataTypeProps Float3		= { GL_FLOAT,  3 * sizeof(GLfloat),  3 };
		static constexpr ShaderDataTypeProps Float4		= { GL_FLOAT,  4 * sizeof(GLfloat),  4 };
		static constexpr ShaderDataTypeProps Mat3		= { GL_FLOAT,  9 * sizeof(GLfloat),  9 };
		static constexpr ShaderDataTypeProps Mat4		= { GL_FLOAT, 16 * sizeof(GLfloat), 16 };
		static constexpr ShaderDataTypeProps Int		= { GL_INT,    1 * sizeof(GLint),    1 };
		static constexpr ShaderDataTypeProps Int2		= { GL_INT,    2 * sizeof(GLint),    2 };
		static constexpr ShaderDataTypeProps Int3		= { GL_INT,    3 * sizeof(GLint),    3 };
		static constexpr ShaderDataTypeProps Int4		= { GL_INT,    4 * sizeof(GLint),    4 };
		static constexpr ShaderDataTypeProps Bool		= { GL_BOOL,   1 * sizeof(GLbyte),   1 };

		friend class BufferElement;
	};

	class BufferElement
	{
	public:
		std::string Name;
		ShaderDataType::ShaderDataTypeProps Type;
		uint32_t Offset;
		bool Normalized;

		BufferElement(ShaderDataType::ShaderDataTypeProps type, const std::string& name, bool normalized = false)
			: Name(name), Type(type), Offset(0), Normalized(normalized)
		{
		}

		inline GLenum GetGLType() const { return Type.type; }
		inline GLenum GetSize() const { return Type.size; }
		inline GLenum GetCount() const { return Type.count; }
	};

	class BufferLayout
	{
	public:
		BufferLayout(const std::initializer_list<BufferElement>& elements)
			: m_Elements(elements)
		{
			CalculateOffsetsAndStride();
		}

		inline uint32_t GetStride() const { return m_Stride; }
		inline const std::vector<BufferElement>& GetElements() const { return m_Elements; }

		std::vector<BufferElement>::iterator begin() { return m_Elements.begin(); }
		std::vector<BufferElement>::iterator end() { return m_Elements.end(); }
		std::vector<BufferElement>::const_iterator begin() const { return m_Elements.begin(); }
		std::vector<BufferElement>::const_iterator end() const { return m_Elements.end(); }
	private:
		void CalculateOffsetsAndStride()
		{
			uint32_t offset = 0;
			m_Stride = 0;
			for (auto& element : m_Elements)
			{
				element.Offset = offset;
				offset += element.GetSize();
				m_Stride += element.GetSize();
			}
		}
	private:
		std::vector<BufferElement> m_Elements;
		uint32_t m_Stride = 0;
	};

	class VertexBuffer
	{
	public:
		static std::shared_ptr<VertexBuffer> Create(float* vertices, uint32_t size);
		~VertexBuffer();

		void Bind() const;
		void Unbind() const;

		const BufferLayout& GetLayout() const { return m_Layout; }
		void SetLayout(const BufferLayout& layout) { m_Layout = layout; }

	private:
		VertexBuffer(float* vertices, uint32_t size);

		uint32_t m_RendererID;
		BufferLayout m_Layout;
	};

	class IndexBuffer
	{
	public:
		static std::shared_ptr<IndexBuffer> Create(uint32_t* indices, uint32_t count);
		~IndexBuffer();

		void Bind() const;
		void Unbind() const;

		inline uint32_t GetCount() const { return m_Count; }

	private:
		IndexBuffer(uint32_t* indices, uint32_t count);

		uint32_t m_RendererID;
		uint32_t m_Count;
	};

}
