
TCP handler (ClientReader)
1) Принимает хэши от клиентов и добавляет их в очередь hashClientQueue вместе с указателем на соединение
2) Когда получает соответствующий запрос от BlockResolver ('Reader from disk with cache')
  a) Отправляет результат
  b) вызывает _Callback_FinishWrite, который очистку памяти, если данными больше никто не пользуется


BlockResolver (Reader from disk with cache)
1) Resolve() занимается обработкой запросов и запускается в отдельном потоке
1) Читает хэши из очереди hashClientQueue 
2) Получив номер блока, ищет его в SendingData или загружает с блочного устройства, если он там не найден
3) Вызывает метод отправки результата у соответствующего соединения

SendingData
1) Хранит данные блока и количество пользователей, которым этот блок отправляется в данный момент
2) GetBlockById()
Ищет в контейнере требуемый блок для последующей отправки и инкрементирует счетчик пользователей, если блок найден
3) DeleteConnection()
Данная функция вызывается через TCP Handler после успешной отправки блока
Декрементирует счетчик получателей данного блока в контейнере отправляемых блоков
Если счетчик равен 0, то удаляет блок из контейнера и освобождает память
4) AddConnectionToBlock()
Вызывается после загрузки блока с помощью get_block_data(...) 
(Ведь блок не был найдет в контейнере и требуется загрузка из блочного устройства)
Добавляет блок в контейнер отправляемых блоков 

