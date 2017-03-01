func insertionSort(array) {
	var i = 1;
	var j;
	var x;
	var break;
	var l = array.length();
	while (i < l) {
		x = array.at(i);
		j = i - 1;
		break = 0;
		while (break == 0) {
			break = 1;
			if (j >= 0) {
				if (array.at(j) > x) {
					array.atPut(j + 1, array.at(j));
					j = j - 1;
					break = 0;
				}
			}
		}
		array.atPut(j + 1, x);
		i = i + 1;
	}
};

func drange(from, to) {
	var array = [];
	var i = from;
	var j = 0;
	while (i > to) {
		array.atPut(j, i);
		i = i - 1;
		j = j + 1;
	}
	return array;
};

var range = drange(500, 0);
insertionSort(range);
