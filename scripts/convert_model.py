#!/usr/bin/env python3
"""
Скрипт конвертации модели HuggingFace в ONNX формат с квантованием.
Используется для подготовки модели T5-small или DistilBART для llmzip.
"""

import argparse
import os
from pathlib import Path

try:
    import torch
    from transformers import T5ForConditionalGeneration, T5TokenizerFast
    from optimum.onnxruntime import ORTModelForSeq2SeqLM, ORTQuantizer
    from optimum.quantization_base import OptimumQuantizer
    from onnxruntime.quantization import QuantType, quantize_dynamic
except ImportError as e:
    print(f"Ошибка импорта: {e}")
    print("Установите зависимости: pip install torch transformers optimum[onnxruntime-gpu] onnx onnxruntime")
    exit(1)

def convert_model(model_name: str, output_dir: str, quantize: bool = True):
    """
    Конвертирует модель из HuggingFace в ONNX формат.
    
    Args:
        model_name: Название модели (например, 'google/flan-t5-small')
        output_dir: Директория для сохранения
        quantize: Применить квантование INT8 для уменьшения размера
    """
    print(f"🚀 Загрузка модели: {model_name}")
    
    # Создаем директорию вывода
    os.makedirs(output_dir, exist_ok=True)
    
    # Загрузка модели и токенизатора
    try:
        model = T5ForConditionalGeneration.from_pretrained(model_name)
        tokenizer = T5TokenizerFast.from_pretrained(model_name)
    except Exception as e:
        print(f"❌ Ошибка загрузки модели: {e}")
        return
    
    # Экспорт в ONNX через optimum
    onnx_model_dir = os.path.join(output_dir, "onnx_model")
    print(f"📦 Экспорт в ONNX: {onnx_model_dir}")
    
    try:
        onnx_model = ORTModelForSeq2SeqLM.from_pretrained(model_name, export=True)
        onnx_model.save_pretrained(onnx_model_dir)
        
        # Сохранение токенизатора
        tokenizer.save_pretrained(onnx_model_dir)
        print("✅ Модель успешно экспортирована в ONNX")
        
    except Exception as e:
        print(f"❌ Ошибка экспорта в ONNX: {e}")
        return
    
    # Квантование (опционально)
    if quantize:
        print("🔧 Применение квантования INT8...")
        quantized_model_path = os.path.join(output_dir, "model_quantized.onnx")
        base_model_path = os.path.join(onnx_model_dir, "decoder_model.onnx")
        
        if os.path.exists(base_model_path):
            try:
                quantize_dynamic(
                    model_input=base_model_path,
                    model_output=quantized_model_path,
                    per_channel=False,
                    reduce_range=False,
                    weight_type=QuantType.QUInt8
                )
                print(f"✅ Квантованная модель сохранена: {quantized_model_path}")
                
                # Проверка размера
                original_size = os.path.getsize(base_model_path) / (1024 * 1024)
                quantized_size = os.path.getsize(quantized_model_path) / (1024 * 1024)
                print(f"📊 Размер оригинала: {original_size:.2f} MB")
                print(f"📊 Размер квантованной: {quantized_size:.2f} MB")
                print(f"💾 Экономия места: {(1 - quantized_size/original_size)*100:.1f}%")
                
            except Exception as e:
                print(f"⚠️ Ошибка квантования: {e}")
                print("Будет использована не-квантованная версия")
        else:
            print(f"⚠️ Файл модели не найден: {base_model_path}")
    
    print(f"\n🎉 Готово! Модель сохранена в: {output_dir}")
    print(f"📁 Для использования в llmzip скопируйте файл model.onnx в папку resources/")

def main():
    parser = argparse.ArgumentParser(description="Конвертер моделей в ONNX для llmzip")
    parser.add_argument(
        "--model", 
        type=str, 
        default="google/flan-t5-small",
        help="Название модели HuggingFace (по умолчанию: google/flan-t5-small)"
    )
    parser.add_argument(
        "--output", 
        type=str, 
        default="./resources",
        help="Директория для сохранения (по умолчанию: ./resources)"
    )
    parser.add_argument(
        "--no-quantize", 
        action="store_true",
        help="Отключить квантование INT8"
    )
    
    args = parser.parse_args()
    
    convert_model(
        model_name=args.model,
        output_dir=args.output,
        quantize=not args.no_quantize
    )

if __name__ == "__main__":
    main()
