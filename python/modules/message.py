import time


class Message:


	def __init__(self):
		self.uuid_log = {}


	def set(self, message):
		pass

	def can_id(self):
		return can_id

	def uuid(self):
		return uuid

	def data(self):
		return data

	def count(self):
		return len(data)

	# add message
	def add(self, message):

		age = 0

		self.purge()

		if (message != False):


			can_id = message["can_id"]
			uuid = message["uuid"]

			# uuid log
			# add uuid id entry
			if not uuid in self.uuid_log:
				self.uuid_log[uuid] = {}

			# add can if not
			if not can_id in self.uuid_log[uuid]:
				self.uuid_log[uuid][can_id] = {}

			# add can id data
			if "time" in self.uuid_log[uuid][can_id]:
				age = self.millis() - self.uuid_log[uuid][can_id]["time"]

			self.uuid_log[uuid][can_id]["data"] = message["data"]
			self.uuid_log[uuid][can_id]["time"] = self.millis()
			self.uuid_log[uuid][can_id]["repeat"] = age


	# get uuid dictionary
	def uuid(self):
		return self.uuid_log


	# get can_id dictionary
	def can_id(self):
		# return self.id_log
		pass


	# purge list
	def purge(self):

		# iterate log
		# for uuid in self.uuid_log:

		# 	# iterate can ids
		# 	for can_id in uuid:

		# 		# entry timed out
		# 		if can_id["age"] > 2000:
		# 			del(uuid)
		# 	pass
		pass


	# get milliseconds
	def millis(self):
		return round(time.time() * 1000)

