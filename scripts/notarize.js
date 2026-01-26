// 文件级关闭
/* eslint-disable */
require('dotenv').config()

// 公证应用本身（在 afterSign hook 中调用）
// 注意：由于 electron-builder.yml 中设置了 mac.notarize: true，
// electron-builder 会自动公证应用，这个 hook 主要用于额外的处理
exports.default = async function notarizing(context) {
  const { electronPlatformName } = context
  if (electronPlatformName !== 'darwin') {
    return
  }

  // electron-builder 的 mac.notarize: true 已经处理了应用公证
  // 这里可以留空，或者添加额外的验证逻辑
  console.log('✅ Application notarization handled by electron-builder (mac.notarize: true)')
}
