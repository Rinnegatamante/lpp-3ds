/**
 * @file news.h
 * @brief NEWS (Notification) service.
 */
#pragma once

typedef struct {
	bool dataSet;
	bool unread;
	bool enableJPEG;
	u8 unkFlag1;
	u8 unkFlag2;
	u64 processID;
	u8 unkData[24];
	u64 time;
	u16 title[32];
} NotificationHeader;

/// Initializes NEWS.
Result newsInit(void);

/// Exits NEWS.
void newsExit(void);

/**
 * @brief Adds a notification to the home menu Notifications applet.
 * @param title UTF-16 title of the notification.
 * @param titleLength Number of characters in the title, not including the null-terminator.
 * @param message UTF-16 message of the notification, or NULL for no message.
 * @param messageLength Number of characters in the message, not including the null-terminator.
 * @param image Data of the image to show in the notification, or NULL for no image.
 * @param imageSize Size of the image data in bytes.
 * @param jpeg Whether the image is a JPEG or not.
 */
Result NEWS_AddNotification(const u16* title, u32 titleLength, const u16* message, u32 messageLength, const void* imageData, u32 imageSize, bool jpeg);

Result NEWS_GetTotalNotifications(u32* num);
Result NEWS_SetNotificationHeader(u32 id, NotificationHeader header);
Result NEWS_GetNotificationImage(u32 id, u8* buffer, u32* size);
Result NEWS_GetNotificationMessage(u32 id, u16* message);
Result NEWS_GetNotificationHeader(u32 id, NotificationHeader* header);