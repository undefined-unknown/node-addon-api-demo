#!/bin/bash

# 构建后验证脚本
# 用于验证构建产物是否正确签名和公证

set -e

echo "🔍 开始验证构建产物..."

# 检查平台
if [[ "$OSTYPE" == "darwin"* ]]; then
  echo "📱 检测到 macOS 平台"
  
  # 查找构建产物
  APP_PATH=$(find dist -name "*.app" -type d | head -n 1)
  DMG_PATH=$(find dist -name "*.dmg" | head -n 1)
  ZIP_PATH=$(find dist -name "*.zip" | head -n 1)
  
  if [ -z "$APP_PATH" ]; then
    echo "❌ 未找到 .app 文件"
    exit 1
  fi
  
  echo "📦 应用路径: $APP_PATH"
  
  # 1. 验证应用签名
  echo ""
  echo "1️⃣  验证应用签名..."
  if codesign -dv --verbose=4 "$APP_PATH" 2>&1 | grep -q "valid on disk"; then
    echo "✅ 应用签名有效"
  else
    echo "❌ 应用签名无效"
    codesign -dv --verbose=4 "$APP_PATH"
    exit 1
  fi
  
  # 2. 验证应用公证
  echo ""
  echo "2️⃣  验证应用公证..."
  if spctl -a -vv -t install "$APP_PATH" 2>&1 | grep -q "accepted"; then
    echo "✅ 应用已公证"
  else
    echo "⚠️  应用未公证或公证未完成"
    spctl -a -vv -t install "$APP_PATH"
  fi
  
  # 3. 验证 DMG 签名（如果存在）
  if [ -n "$DMG_PATH" ]; then
    echo ""
    echo "3️⃣  验证 DMG 签名..."
    if codesign -dv --verbose=4 "$DMG_PATH" 2>&1 | grep -q "valid on disk"; then
      echo "✅ DMG 签名有效"
    else
      echo "❌ DMG 签名无效"
      codesign -dv --verbose=4 "$DMG_PATH"
      exit 1
    fi
    
    echo ""
    echo "4️⃣  验证 DMG 公证..."
    if spctl -a -vv -t install "$DMG_PATH" 2>&1 | grep -q "accepted"; then
      echo "✅ DMG 已公证"
    else
      echo "⚠️  DMG 未公证或公证未完成"
      spctl -a -vv -t install "$DMG_PATH"
    fi
  fi
  
  # 4. 验证 ZIP 文件（用于自动更新）
  if [ -n "$ZIP_PATH" ]; then
    echo ""
    echo "5️⃣  验证 ZIP 文件..."
    if [ -f "$ZIP_PATH" ]; then
      echo "✅ ZIP 文件存在: $ZIP_PATH"
      echo "   大小: $(du -h "$ZIP_PATH" | cut -f1)"
    else
      echo "❌ ZIP 文件不存在"
      exit 1
    fi
  fi
  
  # 5. 验证更新清单文件
  echo ""
  echo "6️⃣  验证更新清单文件..."
  LATEST_YML=$(find dist -name "latest-mac.yml" | head -n 1)
  if [ -n "$LATEST_YML" ]; then
    echo "✅ 找到更新清单: $LATEST_YML"
    echo "   内容预览:"
    head -n 10 "$LATEST_YML" | sed 's/^/   /'
  else
    echo "⚠️  未找到 latest-mac.yml（可能未启用 publish）"
  fi
  
elif [[ "$OSTYPE" == "msys" || "$OSTYPE" == "win32" ]]; then
  echo "🪟 检测到 Windows 平台"
  
  # Windows 验证逻辑
  EXE_PATH=$(find dist -name "*.exe" | head -n 1)
  
  if [ -z "$EXE_PATH" ]; then
    echo "❌ 未找到 .exe 文件"
    exit 1
  fi
  
  echo "📦 安装包路径: $EXE_PATH"
  
  # 验证签名（需要 signtool）
  if command -v signtool &> /dev/null; then
    echo ""
    echo "1️⃣  验证签名..."
    if signtool verify /pa "$EXE_PATH"; then
      echo "✅ 签名有效"
    else
      echo "❌ 签名无效"
      exit 1
    fi
  else
    echo "⚠️  signtool 未找到，跳过签名验证"
  fi
  
else
  echo "❌ 不支持的平台: $OSTYPE"
  exit 1
fi

echo ""
echo "✅ 所有验证完成！"
echo ""
echo "📋 下一步："
echo "   1. 上传文件到更新服务器"
echo "   2. 验证服务器上的文件可访问"
echo "   3. 在旧版本应用中测试自动更新"

