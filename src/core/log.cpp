#include "pch.h"
#include "log.h"
#include "imgui.h"
#include "memory.h"

bool logWindowOpen = true;

#if ENABLE_MESSAGE_LOG

#define MAX_NB_MESSAGES 32

struct log_message
{
	const char* text;
	message_type type;
	float lifetime;
	const char* file;
	const char* function;
	uint32 line;
};

static const ImVec4 colorPerType[] = 
{
	ImVec4(1.f, 1.f, 1.f, 1.f),
	ImVec4(1.f, 1.f, 0.f, 1.f),
	ImVec4(1.f, 0.f, 0.f, 1.f),
};

static eallocator arena;
static std::vector<log_message> messages;
static std::mutex mutex;

void logMessageInternal(message_type type, const char* file, const char* function, uint32 line, const char* format, ...)
{
	mutex.lock();
	arena.ensureFreeSize(1024);

	char* buffer = (char*)arena.getCurrent();

	va_list args;
	va_start(args, format);
	int countWritten = vsnprintf(buffer, 1024, format, args);
	va_end(args);

	messages.push_back({ buffer, type, 5.f, file, function, line });

	arena.setCurrentTo(buffer + countWritten + 1);
	mutex.unlock();
}

void logMessage(message_type type, const char* format, ...)
{
	mutex.lock();
	arena.ensureFreeSize(KB(1));

	char* buffer = (char*)arena.getCurrent();

	va_list args;
	va_start(args, format);
	int countWritten = vsnprintf(buffer, KB(1), format, args);
	va_end(args);

	messages.push_back({ buffer, type, 5.f, nullptr, nullptr, 0 });

	arena.setCurrentTo(buffer + countWritten + 1);
	mutex.unlock();
}

void initializeMessageLog()
{
	arena.initialize(0, MB(128));
}

void updateMessageLog(float dt)
{
	dt = min(dt, 1.f); // If the app hangs, we don't want all the messages to go missing.

	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10.f);
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.f, 0.1f));
	ImGui::SetNextWindowSize(ImVec2(0.f, 0.f)); // Auto-resize to content.
	bool windowOpen = ImGui::Begin("##Console", 0, 
		ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | 
		ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoBringToFrontOnFocus);
	ImGui::PopStyleVar(2);

	uint32 count = (uint32)messages.size();
	uint32 startIndex = 0;

	for (uint32 i = count - 1; i != UINT32_MAX; --i)
	{
		auto& msg = messages[i];

		if (msg.lifetime <= 0.f)
		{
			startIndex = i + 1;
			break;
		}
		msg.lifetime -= dt;
	}

	uint32 numMessagesToShow = count - startIndex;
	numMessagesToShow = min(numMessagesToShow, 8u);
	startIndex = count - numMessagesToShow;

	ImGui::End();

	if (logWindowOpen)
	{
		if (ImGui::Begin(ICON_FA_CLIPBOARD_LIST "  Console", &logWindowOpen))
		{
			for (uint32 i = 0; i < count; ++i)
			{
				auto& msg = messages[i];
				if(msg.file)
					ImGui::TextColored(colorPerType[msg.type], "%s (%s [%u])", msg.text, msg.function, msg.line);
				else
					ImGui::TextColored(colorPerType[msg.type], "%s", msg.text);
			}
		}
		ImGui::End();
	}

	if (messages.size() > MAX_NB_MESSAGES)
	{
		messages.clear();
		arena.reset();
	}
}

#endif
