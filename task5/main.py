import argparse
import threading
import queue
import time

from ultralytics import YOLO  # для обнаружения объектов
import cv2  # для работы с изображениями и видео


# Функция для чтения кадров из видео и добавления их в очередь
def fun_thread_read(path_video: str, frame_queue: queue.Queue, event_stop: threading.Event):
    cap = cv2.VideoCapture(path_video)  # Открываем видеофайл
    ind = 0  # Инициализируем индекс кадра
    while cap.isOpened():  # Пока видеофайл открыт
        ret, frame = cap.read()  # Читаем кадр из видео
        if not ret:  # Если не удалось получить кадр
            print("Не удается получить кадр!")
            break  # Выходим из цикла
        frame_queue.put((frame, ind))  # Добавляем кадр в очередь
        ind += 1  # Увеличиваем индекс кадра
        time.sleep(0.0001)  # Небольшая задержка для управления процессорным временем
    event_stop.set()  # Устанавливаем событие завершения работы


# Функция для записи кадров из очереди в видеофайл
def fun_thread_write(length: int, fps: int, out_queue: queue.Queue, out_path: str):
    t = threading.current_thread()  # Получаем текущий поток
    frames = [None] * length  # Создаем список для кадров

    while getattr(t, "do_run", True):  # Пока флаг do_run равен True
        try:
            frame, ind = out_queue.get(timeout=1)  # Получаем кадр из очереди
            frames[ind] = frame  # Добавляем кадр в список по соответствующему индексу
        except queue.Empty:  # Если очередь пуста
            pass
    print("Остановка записи.")

    print("Начало компоновки.")
    width, height = frames[0].shape[1], frames[0].shape[0]  # Получаем ширину и высоту кадра
    out = cv2.VideoWriter(out_path, cv2.VideoWriter_fourcc(*'mp4v'), fps, (width, height))  # Создаем объект для записи видео

    for frame in frames:  # Для каждого кадра в списке
        out.write(frame)  # Записываем кадр в видеофайл
    out.release()  # Освобождаем объект записи видео


# Функция для безопасного обнаружения объектов на кадрах из очереди
def fun_thread_safe_predict(frame_queue: queue.Queue, out_queue: queue.Queue, event_stop: threading.Event):
    model = YOLO(model="yolov8s-pose.pt", verbose=False)  # Создаем экземпляр модели YOLO
    while True:
        try:
            frame, ind = frame_queue.get(timeout=1)  # Получаем кадр из очереди
            results = model.predict(source=frame, device='cpu')[0].plot()  # Обнаруживаем объекты на кадре
            out_queue.put((results, ind))  # Добавляем результаты в очередь
        except queue.Empty:  # Если очередь пуста
            if event_stop.is_set():  # Если установлен флаг завершения работы
                print(f'Поток {threading.get_ident()} завершен!')
                break  # Завершаем выполнение функции


# Основная функция программы
def main(arg):
    threads = []  # Список для хранения потоков
    frame_queue = queue.Queue(1000)  # Очередь для кадров
    out_queue = queue.Queue()  # Очередь для результатов
    event_stop = threading.Event()  # Событие для сигнала о завершении работы
    video_path = arg.inp  # Путь к входному видео
    cap = cv2.VideoCapture(video_path)  # Открываем входное видео
    length = int(cap.get(cv2.CAP_PROP_FRAME_COUNT))  # Получаем количество кадров в видео
    fps = cap.get(cv2.CAP_PROP_FPS)  # Получаем частоту кадров в видео
    cap.release()  # Закрываем входное видео

    # Запускаем поток для чтения кадров из видео
    thread_read = threading.Thread(target=fun_thread_read, args=(video_path, frame_queue, event_stop,))
    thread_read.start()

    # Запускаем поток для записи кадров в видеофайл
    thread_write = threading.Thread(target=fun_thread_write, args=(length, fps, out_queue, arg.out,))
    thread_write.start()

    start_t = time.monotonic()  # Запускаем отсчет времени

    # Создаем и запускаем несколько потоков для безопасного обнаружения объектов
    for _ in range(arg.th):
        threads.append(threading.Thread(target=fun_thread_safe_predict, args=(frame_queue, out_queue, event_stop,)))

    for thr in threads:  # Для каждого потока
        thr.start()  # Запускаем поток

    for thr in threads:  # Для каждого потока
        thr.join()  # Ожидаем завершения потока

    thread_read.join()  # Ожидаем завершения потока чтения

    # Останавливаем поток записи
    thread_write.do_run = False
    thread_write.join()  # Ожидаем завершения потока записи

    end_t = time.monotonic()  # Завершаем отсчет времени
    print(f'Время работы: {end_t - start_t}')  # Выводим время выполнения программы


if __name__ == "__main__":
    parser = argparse.ArgumentParser()  # Создаем парсер аргументов командной строки
    parser.add_argument('--inp', type=str, default='input.mp4', help='input/path/to/video.mp4')
    parser.add_argument('--out', type=str, default='output.mp4', help='output/path/to/result.mp4')
    parser.add_argument('--th', type=int, default=1, help='number thread')
    args = parser.parse_args()  # Парсим аргументы командной строки
    main(args)  # Запускаем основную функцию
